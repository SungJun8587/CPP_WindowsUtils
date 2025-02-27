@ECHO OFF
:: ShCopyMove.exe [이동(M)/복사(C)] [원본 폴더] [대상 폴더] [적용,제외 여부] [확장자] [시작 일시] [종료 일시]
:: [설명]
::  - 원본 폴더에 하위 폴더와 파일들을 대상 폴더에 이동 또는 복사
::  - 대상 폴더는 존재하지 않아도 하위 디렉터리까지 생성한 후에 이동 또는 복사
::  - 적용,제외 여부 파라미터가 1이면 확장자 파일만 적용, 0이면 확장자 파일 제외(명시하지 않으면 모든 파일 적용) 
::  - 시작 일시와 종료 일시 사이에 수정된 파일만 적용(명시하지 않으면 모든 파일 적용)
:: [파라미터 설명]
::      - 1. 이동/복사 : M/C
::      - 2. 원본 폴더 : E:\GitHub\CPP\Library\DataStructures
::      - 3. 대상 폴더 : E:\Backup\CPP\Library\DataStructures
::      - 4. 적용,제외 여부 : 1/0
::      - 5. 확장자 : 해당 확장자만 적용(Ex. ".asp;.htm;.html")
::      - 6. 시작 일시 : 수정 일시을 기준으로 시작 일시
::      - 7. 종료 일시 : 수정 일시을 기준으로 종료 일시
x64\Release\ShCopyMove.exe C "E:\GitHub\CPP\Library\DataStructures" "E:\Backup\CPP\Library\DataStructures" 1 ".cpp;.h" "2017-08-15 00:00:00" "2017-08-15 23:59:59"