@ECHO OFF
:: FileInfoScanner.exe [대상 경로] [출력 모드]
:: [설명]
::  - 대상 경로 하위의 모든 폴더/파일을 재귀 탐색하여 파일 정보(폴더, 파일명, 크기, 생성일시, 수정일시, 인코딩 타입)를 수집
::  - 출력 모드가 1이면 대상 경로 안에 FileInfoResult.xlsx로 저장, 0이면 콘솔에 표 형태로 출력(명시하지 않으면 콘솔 출력)
:: [파라미터 설명]
::      - 1. 대상 경로 : E:\GitHub\CPP\Library\DataStructures
::      - 2. 출력 모드 : 0/1(콘솔 출력/엑셀 저장, 생략 시 0)
x64\Release\FileInfoScanner.exe "E:\GitHub\CPP\Library\DataStructures" 1
