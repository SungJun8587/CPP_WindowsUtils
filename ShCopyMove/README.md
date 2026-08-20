# ShCopyMove

싱글 생산자 - 멀티 소비자(SPMC: Single-Producer Multi-Consumer) 패턴을 이용한 고성능 병렬 파일 복사/이동 콘솔 프로그램. `std::filesystem` 기반으로 동작하여 Windows/Linux/macOS에서 동일하게 빌드된다.

## 개요

- 원본 폴더 하위의 폴더 및 파일 전체를 대상 폴더로 이동(Move) 또는 복사(Copy)한다.
- 대상 폴더는 존재하지 않아도 하위 디렉터리까지 자동으로 생성한 뒤 이동/복사한다.
- 확장자 화이트리스트/블랙리스트 필터, 날짜(생성일·수정일) 범위 필터를 지원한다(필터 판정은 `DirectoryUtil.h`의 `SH_APPLY_FILEINFO`/`IsAbleFile`이 담당).
- 메인 스레드(생산자) 1개가 디렉터리 트리를 탐색하며 파일 작업을 큐에 적재하고, 다수의 소비자 스레드가 큐에서 작업을 꺼내 병렬로 실제 복사/이동을 수행한다.
- 디렉터리 열람/파일 상태 조회 중 발생하는 오류는 조용히 넘기지 않고 로그로 남기고 집계하여, 원본과 결과물의 개수가 달라질 수 있는 상황을 항상 파악할 수 있게 한다.

## 아키텍처

```
[메인 스레드: Producer]                    [Worker Thread Pool: Consumer x N]
DirectoryRecursiveSearch()                  ConsumerFunc()
  ├─ fs::directory_iterator로 탐색            ├─ taskQueue.PopChunk(64개씩)
  ├─ 필터 통과 파일 -> taskQueue.Push()         ├─ ProcessSingleFile() 실행
  └─ 폴더 탐색 종료 -> ReleaseDirNode()         └─ ReleaseDirNode()로 참조 해제
       │
       ▼
CChunkedBlockingQueue<FileTask> (백프레셔 10,000개)
```

- 생산자와 소비자는 `CChunkedBlockingQueue<FileTask>`를 통해 통신한다. 큐가 최대치(10,000개)에 도달하면 생산자가 블로킹되어 무제한 메모리 증가를 방지한다.
- 소비자는 파일 하나씩이 아니라 `PopChunk(64)`로 청크 단위로 가져와 락 경합을 줄인다.

## 문자열/로그 처리

- `_tstring`(UNICODE 여부에 따라 `std::string`/`std::wstring`)을 프로젝트 전반의 문자열 타입으로 사용한다.
- `LOG_INFO`/`LOG_WARNING`/`LOG_ERROR`(`CLogManager`)로 모든 상태·오류 메시지를 남긴다. 이 파일 안에서만 `LOG_INFO`를 시간 표시 없이 찍도록 재정의해서 사용한다(`Log.h` 자체는 건드리지 않음).
- `std::error_code`/`std::exception`의 메시지는 시스템 로케일에 따라 항상 narrow(`char`)로 나오므로, `ErrorToString()`/`ExceptionToString()` 헬퍼로 `_UNICODE` 환경에서는 `_tstring`(wide)으로, 아니면 그대로 반환하여 로그 매크로에 안전하게 넘긴다.
- 프로그램 시작 시 `setlocale(LC_ALL, ".UTF8")`과 `SetConsoleOutputCP/SetConsoleCP(CP_UTF8)`(Windows 한정)로 콘솔 입출력을 UTF-8로 고정해 한글이 깨지지 않게 한다.

## 주요 컴포넌트

### DirNode — 소스 폴더 참조 카운트 노드 (FO_MOVE 전용)

이동 모드에서 빈 폴더를 자동 삭제하기 위한 참조 카운트 구조체.

- `pendingCount`는 1(자체 탐색-진행-중 토큰)로 시작하며, 파일/하위폴더를 발견할 때마다 +1, 각 참조가 처리 완료될 때마다 -1 한다.
- 0이 되는 순간(탐색도 끝났고 하위 파일/폴더도 모두 처리된 시점) 자기 자신을 삭제하고, 부모 노드에도 동일하게 전파한다(연쇄 삭제).
- 루트 폴더(`isRoot == true`)는 삭제 대상에서 제외된다.
- 경로는 `fs::path`로 보관하여 인코딩 문제 없이 플랫폼 간 이식된다.

### FileTask

개별 파일 작업 단위. `srcPath`, `destPath`(둘 다 `fs::path`), `nFunc`(FO_COPY/FO_MOVE), FO_MOVE일 때만 사용되는 `dirNode` 포인터를 담는다.

### FileProcessContext

생산자/소비자 스레드 간 공유되는 상태 컨텍스트.

| 필드 | 설명 |
|---|---|
| `taskQueue` | 파일 작업 큐 (최대 10,000개 백프레셔) |
| `allSuccess` | 전체 작업 성공 여부 |
| `fileSuccessCount` / `fileFailCount` | 파일 처리 성공/실패 수 |
| `folderCount` | 매칭 파일이 하나라도 있었던 소스 폴더 수 |
| `deletedFolderCount` | 실제로 삭제된 빈 소스 폴더 수 (FO_MOVE 전용) |
| `scanErrorCount` | 디렉터리 열람 또는 파일 상태 조회 실패로 일부/전체 항목을 건너뛴 횟수. 이 값이 0보다 크면 원본과 결과물의 개수가 다를 수 있다는 뜻이다 |
| `createdFoldersMutex` / `createdFolders` | 대상 폴더 생성 여부를 캐시하는 `shared_mutex` + `unordered_set<fs::path::string_type>`. 조회(읽기)가 압도적으로 많고 실제 생성(쓰기)은 드물기 때문에 `shared_mutex`로 보호하여, 조회 시 `shared_lock`으로 여러 스레드가 동시에 캐시를 읽을 수 있도록 한다 |
| `createdFolderCount` | 실제로 새로 생성된 대상 폴더 수 |

## 핵심 함수

### ReleaseDirNode(node, ctx)

DirNode 참조를 하나 해소한다. 마지막 참조라면 해당 폴더를 삭제하고 부모로 전파한다. 여러 컨슈머 스레드에서 동시 호출되어도 안전하다(atomic fetch_sub 기반).

### EnsureDestFolder(destFolder, ctx)

대상 폴더가 없으면 상위 → 하위 순으로 세그먼트를 생성한다. 여러 소비자 스레드가 동시에 같은 경로를 생성하려 해도 정확히 1번만 카운트된다.

- **빠른 경로**: 이미 생성이 확인된 폴더는 `createdFolders` 캐시를 `shared_lock`으로 조회하는 것만으로 즉시 반환한다. 파일시스템 syscall(`fs::exists`) 없이 메모리 조회만으로 끝나므로, 같은 대상 폴더에 파일이 대량으로 몰리는 경우에도 폴더당 실질적인 디스크 접근은 최초 1회뿐이다.
- 캐시 미스 시에만 `unique_lock`을 잡고, 락 획득 사이의 경합에 대비해 캐시를 한 번 더 확인(더블 체크)한 뒤 실제 `fs::exists`/`fs::create_directory`를 수행한다.

### ProcessSingleFile(src, dest, nFunc, ctx)

단일 파일의 복사/이동을 처리한다.

- FO_MOVE는 우선 `fs::rename()`을 시도한다. 같은 볼륨 내 이동이면 원자적으로 처리되어 빠르고 중간 상태가 남지 않는다.
- `rename()`이 실패하는 대표 케이스(다른 드라이브/볼륨 간 이동)에는 `copy_file()` + `remove()`로 폴백한다. 이때 `remove()`의 성공 여부까지 확인하여, 복사는 됐지만 원본 삭제가 실패한 상태는 실패로 보고한다(중복 파일 방지).
- 예외 발생 시 `LOG_ERROR`로 원인을 남긴다.

### DirectoryRecursiveSearch(...)

지정한 소스 폴더를 재귀적으로 탐색하며 조건에 맞는 파일을 찾아 큐에 적재하는 생산자 측 핵심 함수.

- `fs::directory_iterator(sourceFolder, fs::directory_options::skip_permission_denied, dirEc)`로 디렉터리를 탐색한다. `.`/`..` 항목은 애초에 반환되지 않으므로 별도 필터링이 필요 없다.
- **오류를 조용히 넘기지 않는다**: 다음 세 지점 각각에서 실패가 발생하면 `LOG_WARNING`으로 남기고 `scanErrorCount`를 증가시키며 `allSuccess`를 `false`로 표시한다.
  1. 디렉터리 자체를 열람하지 못하는 경우(`dirEc`) — 권한/경로 문제 등
  2. 개별 항목의 상태 조회(`entry.is_directory()`)가 실패하는 경우(`typeEc`) — 클라우드 동기화 placeholder, 나열 직후 삭제된 파일, 끊어진 심볼릭 링크 등
  3. 순회 도중 다음 항목으로 넘어가지 못하는 경우(`it.increment()` 실패) — 표준 규격상 실패 시 iterator가 곧바로 `end`가 되어버리므로, **처리 직후·`increment` 호출 바로 뒤에서** 확인해야 한다(루프 조건에서 확인하면 이미 관측 기회를 놓친 뒤다)
- 재귀 무한 루프 방지를 위해 심볼릭 링크/정션은 하위 탐색에서 제외한다(파일 자체는 그대로 처리).
- 필터 조건(`IsAbleFile`: 확장자 화이트/블랙리스트 + 날짜 범위)을 통과한 파일만 큐에 적재한다.
- 이 폴더에서 처음 매칭된 파일이 나오는 시점에 `folderCount`를 1회 증가시킨다(폴더당 중복 집계 방지).
- FO_MOVE일 때는 파일/하위폴더를 발견할 때마다 `DirNode::pendingCount`를 증가시켜, 처리 완료 전까지 상위 폴더가 삭제되지 않도록 보장한다.

### ProducerFunc / ConsumerFunc

- `ProducerFunc`: 메인 스레드에서 `DirectoryRecursiveSearch`로 트리 탐색을 전담하고, 완료 시 `taskQueue.SetProducerDone()`으로 대기 중인 모든 소비자 스레드를 깨운다.
- `ConsumerFunc`: 큐에서 64개 단위 청크를 꺼내 순차적으로 `ProcessSingleFile`을 호출하고, 성공/실패를 집계하며 `ReleaseDirNode`로 폴더 참조를 해제한다.

## 필터 판정 (DirectoryUtil.h / SH_APPLY_FILEINFO)

`ShCopyMove.cpp`는 필터 구조체와 판정 함수를 직접 갖지 않고 `DirectoryUtil.h`의 `SH_APPLY_FILEINFO`/`IsAbleFile`을 그대로 사용한다.

- **확장자 필터** (`m_nFilterMode`): `0`=미적용, `1`=화이트리스트(지정 확장자만 허용), `2`=블랙리스트(지정 확장자 제외)
- **날짜 필터** (`m_tszModifyStDate`/`m_tszModifyEdDate`, YYYYMMDD): 시작일/종료일이 둘 다 비어 있으면 미적용. 값이 있으면:
  - **Windows**: 파일의 생성일 또는 수정일 중 **하나라도** 범위 안에 들면 허용
  - **그 외 플랫폼**: 수정일만 기준으로 판정(POSIX는 파일 생성일(`birth time`)을 표준으로 보장하지 않으므로 사용하지 않음)
- 두 필터는 AND로 결합된다 — 확장자 조건과 날짜 조건을 모두 통과해야 최종 허용된다.

## 실행 파라미터

```
ShCopyMove.exe [모드] [원본경로] [대상경로] [필터적용여부] [확장자필터] [시작일] [종료일] [소비자스레드수]
```

| # | 인자 | 설명 |
|---|---|---|
| 1 | 모드 | `C`: 복사(FO_COPY), `M`: 이동(FO_MOVE). 이동 시 소스 하위 폴더에 파일이 남지 않으면 자동 삭제(최상위 소스 폴더 자체는 삭제하지 않음) |
| 2 | 원본경로 | 복사/이동할 원본 폴더(또는 파일)의 전체 경로. 경로에 공백이 있으면 `;32;`로 치환(예: `C:\My;32;Folder`) |
| 3 | 대상경로 | 목적지 폴더 전체 경로. 존재하지 않으면 자동 생성 |
| 4 | 필터적용여부 | `0`: 필터링 없음, `1`: 화이트리스트(지정 확장자만 허용), `2`: 블랙리스트(지정 확장자 제외). 숫자가 아니면 오류 메시지를 남기고 종료 |
| 5 | 확장자필터 | 예: `"txt"`, 복수 지정 시 세미콜론 구분(`"txt;log;csv"`). 미사용 시 `""` |
| 6 | 시작일 | 날짜 필터 시작일(YYYYMMDD, 예: `20260101`). Windows에서는 생성일 또는 수정일 중 하나라도 범위에 들면 허용 |
| 7 | 종료일 | 날짜 필터 종료일(YYYYMMDD, 예: `20260807`) |
| 8 | 소비자 스레드 수 (선택) | 미지정 또는 `0`이면 `SYSTEM::CoreCount()`로 자동 설정. 숫자가 아니면 오류 메시지를 남기고 종료. 원본/대상 저장장치 종류(SSD/HDD/네트워크 드라이브)에 따라 직접 조정 가능 |

## 사용 예제

### 콘솔 직접 실행

```
ShCopyMove.exe C "C:\Source" "D:\Dest" 1 "txt" "20260101" "20260807"
ShCopyMove.exe M "C:\Work" "D:\Archive" 0 "" "" ""
ShCopyMove.exe C "C:\My;32;Documents" "D:\Backup" 0 "" "" ""
ShCopyMove.exe C "C:\Source" "D:\Dest" 0 "" "" "" 8
```

### 배치 스크립트(.bat) 예제

원본/대상 폴더와 확장자, 날짜 범위를 지정해 특정 기간에 수정된 `.cpp`/`.h` 파일만 백업 폴더로 복사하는 예제.

```bat
@ECHO OFF
:: ShCopyMove.exe [이동(M)/복사(C)] [원본 폴더] [대상 폴더] [적용,제외 여부] [확장자] [시작 일시] [종료 일시]
:: [설명]
::  - 원본 폴더에 하위 폴더와 파일들을 대상 폴더에 이동 또는 복사
::  - 대상 폴더는 존재하지 않아도 하위 디렉터리까지 생성한 후에 이동 또는 복사
::  - 적용,제외 여부 파라미터가 1이면 확장자 파일만 적용, 0이면 확장자 파일 제외(명시하지 않으면 모든 파일 적용)
::  - 시작 일시와 종료 일시 사이에 생성/수정된 파일만 적용(명시하지 않으면 모든 파일 적용)
:: [파라미터 설명]
::      - 1. 이동/복사 : M/C
::      - 2. 원본 폴더 : E:\GitHub\CPP\Library\DataStructures
::      - 3. 대상 폴더 : E:\Backup\CPP\Library\DataStructures
::      - 4. 적용,제외 여부 : 0/1/2(전체 허용/지정한 확장자만 허용/지정한 확장자는 제외)
::      - 5. 확장자 : 해당 확장자만 적용(Ex. "asp;htm;html")
::      - 6. 시작 일시 : 생성일 또는 수정일 기준 시작 일시
::      - 7. 종료 일시 : 생성일 또는 수정일 기준 종료 일시
::      - 8. 소비자 스레드 수 : 파일 복사/이동을 병렬로 처리할 소비자(Consumer) 스레드 개수(값을 지정하지 않거나 0을 입력하면 시스템 프로세서 코어 개수(SYSTEM::CoreCount())로 자동 설정)
x64\Release\ShCopyMove.exe C "E:\GitHub\CPP\Library\DataStructures" "E:\Backup\CPP\Library\DataStructures" 1 "cpp;h" "2017-08-15 00:00:00" "2017-08-15 23:59:59" 0
```

이 예제는 `DataStructures` 폴더 전체를 대상으로, 2017-08-15 하루 동안 생성/수정된 `.cpp`/`.h` 파일만 골라 `Backup` 경로로 복사하며, 소비자 스레드 수는 `0`(자동 = CPU 코어 수)으로 지정한다.

### 디버깅용 인자 오버라이드(F5 실행)

Visual Studio에서 `_DEBUG` 빌드로 F5 실행 시 `mockArgv` 배열의 값이 강제 적용되어 실제 명령행 인자를 무시한다. 실제 인자로 테스트하려면 프로젝트 속성 → 디버깅 → 명령 인수에 위 예제 문자열을 입력하거나, 릴리즈 빌드로 실행 시 명령행 인자를 그대로 전달하면 된다.

## 주의사항

- 경로에 공백이 포함된 경우 공백 대신 반드시 `;32;`를 사용해야 한다(콘솔 인자 파싱 특성상).
- 재귀 탐색 시 심볼릭 링크/정션은 무한 루프 방지를 위해 제외된다.
- FO_MOVE에서 `rename()`이 실패해 `copy + remove`로 폴백하는 경우, 원본 삭제까지 성공해야 최종 성공으로 처리된다. 삭제만 실패하면 중복 파일이 남을 수 있으므로 실패로 집계된다.
- 큐 백프레셔가 10,000개로 설정되어 있어, 소비자 처리 속도보다 탐색 속도가 훨씬 빠른 경우(파일 수가 매우 많은 트리) 생산자가 일시적으로 블로킹될 수 있다. 필요 시 `CChunkedBlockingQueue<FileTask> taskQueue{ 10000 }`의 상한값을 조정한다.
- **원본과 결과물의 폴더/파일 개수가 다르면 가장 먼저 실행 로그의 `scanErrorCount`(또는 `[경고]` 문구)와 날짜 필터 인자(6, 7번)를 확인한다.** 스캔 오류가 0인데도 개수가 다르다면, 지정한 날짜 범위가 의도한 것보다 좁게 걸려 있어 정상적으로 필터링된 결과일 수 있다.
