#include <iostream>
#include <unordered_set>
#include <sstream>
#include <string>

using namespace std;

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int n, m;
    cin >> n >> m;

    unordered_set<string> obstacles;

    // 读取障碍物的位置
    for (int i = 0; i < n; ++i) {
        int x, y;
        cin >> x >> y;
        stringstream ss;
        ss << x << "," << y;
        obstacles.insert(ss.str());
    }

    // 机器人初始位置
    int x = 0, y = 0;

    // 读取并处理每条指令
    for (int i = 0; i < m; ++i) {
        char direction;
        int distance;
        cin >> direction >> distance;

        int dx = 0, dy = 0;

        // 确定方向向量
        switch (direction) {
            case 'U': dy = 1; break;
            case 'D': dy = -1; break;
            case 'L': dx = -1; break;
            case 'R': dx = 1; break;
        }

        bool valid = true;

        // 检查移动路径是否有障碍物
        for (int step = 1; step <= distance; ++step) {
            int newX = x + dx * step;
            int newY = y + dy * step;
            stringstream ss;
            ss << newX << "," << newY;
            if (obstacles.find(ss.str()) != obstacles.end()) {
                valid = false;
                break;
            }
        }

        // 如果没有障碍物，则更新位置
        if (valid) {
            x += dx * distance;
            y += dy * distance;
        }
    }

    // 输出最终位置
    cout << x << " " << y << endl;

    return 0;
}
