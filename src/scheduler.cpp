#include "scheduler.h"
#include <sstream>
#include <iomanip>
#include <ctime>

// "2026-02-25T18:00:00" 형식 문자열 → Unix timestamp 변환
static long long parseDeadline(const std::string& s) {
    std::tm tm = {};
    std::istringstream ss(s);
    ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");
    tm.tm_isdst = -1;
    return (long long)mktime(&tm);
}

// JSON 바디에서 Task 목록 파싱
std::vector<Task> Scheduler::parseTasks(const crow::json::rvalue& body) {
    std::vector<Task> tasks;

    for (auto& item : body["tasks"]) {
        Task t;
        t.id           = item["id"].i();
        t.title        = item["name"].s();          // "name"으로 수정
        t.duration_min = item["duration"].i();       // "duration"으로 수정
        t.priority     = item["priority"].i();
        t.deadline     = parseDeadline(item["deadline"].s()); // 문자열 → timestamp
        tasks.push_back(t);
    }

    return tasks;
}

// EDF + 우선순위 가중치 기반 스케줄링
std::vector<ScheduledBlock> Scheduler::schedule(std::vector<Task>& tasks, long long available_start) {
    std::priority_queue<Task, std::vector<Task>, std::greater<Task>> pq;
    for (auto& t : tasks) {
        pq.push(t);
    }

    std::vector<ScheduledBlock> result;
    long long current_time = available_start;

    while (!pq.empty()) {
        Task t = pq.top();
        pq.pop();

        ScheduledBlock block;
        block.task       = t;
        block.start_time = current_time;
        block.end_time   = current_time + (t.duration_min * 60);

        result.push_back(block);
        current_time = block.end_time;
    }

    return result;
}

// 외부에서 호출하는 메인 함수
crow::json::wvalue Scheduler::optimize(const crow::json::rvalue& body) {
    auto tasks = parseTasks(body);

    long long start_time = body.has("available_start")
        ? body["available_start"].i()
        : (long long)time(nullptr);

    auto scheduled = schedule(tasks, start_time);

    crow::json::wvalue result;
    result["status"] = "ok";
    result["count"]  = (int)scheduled.size();

    std::vector<crow::json::wvalue> blocks;
    for (auto& b : scheduled) {
        crow::json::wvalue block;
        block["id"]         = b.task.id;
        block["title"]      = b.task.title;
        block["start_time"] = b.start_time;
        block["end_time"]   = b.end_time;
        block["priority"]   = b.task.priority;
        blocks.push_back(std::move(block));
    }
    result["schedule"] = std::move(blocks);

    return result;
}