#pragma once
#include <vector>
#include <string>
#include <queue>
#include <algorithm>
#include "crow_all.h"

struct Task {
    int id;
    std::string title;
    int duration_min;    // 예상 소요 시간 (분)
    int priority;        // 1(낮음) ~ 5(높음)
    long long deadline;  // Unix timestamp

    // 우선순위 큐 정렬 기준: 마감일 + 우선순위 가중치
    bool operator>(const Task& other) const {
        long long score = deadline - (priority * 3600);
        long long other_score = other.deadline - (other.priority * 3600);
        return score > other_score;
    }
};

struct ScheduledBlock {
    Task task;
    long long start_time;  // Unix timestamp
    long long end_time;
};

class Scheduler {
public:
    // JSON으로 받은 태스크들을 최적 순서로 스케줄링
    crow::json::wvalue optimize(const crow::json::rvalue& body);

private:
    std::vector<Task> parseTasks(const crow::json::rvalue& body);
    std::vector<ScheduledBlock> schedule(std::vector<Task>& tasks, long long available_start);
};