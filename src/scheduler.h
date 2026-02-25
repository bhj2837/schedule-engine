#pragma once
#include <vector>
#include <string>
#include <queue>
#include <algorithm>
#include <mutex>
#include <optional>
#include "crow_all.h"

// ───────────────────────────────────────────
// 도메인 구조체
// ───────────────────────────────────────────
struct Task {
    int id;
    std::string title;
    int duration_min;
    int priority;
    long long deadline;

    bool operator>(const Task& other) const {
        long long score       = deadline - (priority * 3600LL);
        long long other_score = other.deadline - (other.priority * 3600LL);
        return score > other_score;
    }
};

struct ScheduledBlock {
    Task task;
    long long start_time;
    long long end_time;
};

// ───────────────────────────────────────────
// 에러 응답 헬퍼
// ───────────────────────────────────────────
inline crow::response errorResponse(int code, const std::string& message) {
    crow::json::wvalue body;
    body["status"]  = "error";
    body["message"] = message;
    return crow::response(code, body.dump());
}

// ───────────────────────────────────────────
// 인메모리 태스크 저장소 (전역 싱글턴)
// ───────────────────────────────────────────
class TaskStore {
public:
    // 태스크 upsert (같은 id면 덮어쓰기)
    void upsert(const Task& task) {
        std::lock_guard<std::mutex> lock(mtx_);
        for (auto& t : tasks_) {
            if (t.id == task.id) { t = task; return; }
        }
        tasks_.push_back(task);
    }

    // 벡터 복사본 반환 (읽기 전용 스냅샷)
    std::vector<Task> getAll() const {
        std::lock_guard<std::mutex> lock(mtx_);
        return tasks_;
    }

    static TaskStore& instance() {
        static TaskStore store;
        return store;
    }

private:
    TaskStore() = default;
    mutable std::mutex mtx_;
    std::vector<Task> tasks_;
};

// ───────────────────────────────────────────
// 스케줄러
// ───────────────────────────────────────────
class Scheduler {
public:
    // 성공 시 wvalue, 실패 시 crow::response 반환
    // → std::variant 대신 간단하게 optional + out-param 방식 사용
    bool optimize(const crow::json::rvalue& body,
                  crow::json::wvalue& out,
                  crow::response&    err);

private:
    // 실패 시 errMsg에 메시지를 담고 false 반환
    bool parseTasks(const crow::json::rvalue& body,
                    std::vector<Task>&        out,
                    std::string&              errMsg);

    std::vector<ScheduledBlock> schedule(std::vector<Task>& tasks,
                                         long long available_start);
};