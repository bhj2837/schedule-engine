# schedule-engine

C++ 기반 스마트 일정 최적화 REST API 서버

## Overview

EDF(Earliest Deadline First) 알고리즘에 우선순위 가중치를 결합한 태스크 스케줄링 엔진입니다.  
마감일과 중요도를 함께 고려하여 최적의 작업 순서를 계산하고, REST API로 결과를 반환합니다.

## Tech Stack

| 항목 | 내용 |
|------|------|
| Language | C++17 |
| Framework | [Crow](https://github.com/CrowCpp/Crow) |
| Network | Asio 1.28.0 (standalone) |
| Build | CMake |
| Compiler | MSVC (Visual Studio 2022) |
| OS | Windows 10 |

## Algorithm

### EDF + Priority Weight Scheduling

단순 EDF는 마감일만 보지만, 이 엔진은 **우선순위 가중치**를 추가로 반영합니다.

```
score = deadline - (priority × 3600)
```

score가 낮을수록 먼저 처리됩니다. 마감일이 같다면 priority가 높은 태스크가 앞으로 당겨집니다.

