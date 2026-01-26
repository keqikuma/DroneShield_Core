#ifndef APPSTYLE_H
#define APPSTYLE_H

#include <QString>

inline QString getDarkTacticalStyle() {
    return R"(
        /* 全局背景：深灰黑 */
        QMainWindow {
            background-color: #121212;
        }

        /* 分组框：绿色边框，科技感 */
        QGroupBox {
            border: 1px solid #333333;
            border-radius: 4px;
            margin-top: 20px; /* 为标题留空间 */
            font-weight: bold;
            color: #aaaaaa;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            subcontrol-position: top left;
            padding: 0 5px;
            color: #00ff00; /* 标题亮绿 */
            left: 10px;
        }

        /* 列表表格：黑底绿字，类似终端 */
        QTableWidget {
            background-color: #000000;
            gridline-color: #333333;
            color: #00ff00;
            border: 1px solid #444;
            font-family: "Consolas", "Monospace";
        }
        QHeaderView::section {
            background-color: #222222;
            color: white;
            padding: 4px;
            border: 1px solid #333;
        }

        /* 选中行：高亮 */
        QTableWidget::item:selected {
            background-color: #003300;
            color: #ffffff;
        }

        /* 按钮：战术风格 */
        QPushButton {
            background-color: #1e1e1e;
            border: 1px solid #444;
            color: #cccccc;
            padding: 8px;
            border-radius: 2px;
            font-size: 14px;
        }
        QPushButton:hover {
            border: 1px solid #00ff00;
            color: #00ff00;
        }
        QPushButton:checked {
            background-color: #004400; /* 选中时变成深绿背景 */
            border: 1px solid #00ff00;
            color: #ffffff;
        }

        /* 自动模式大按钮 */
        QPushButton#btnAutoMode {
            font-size: 16px;
            font-weight: bold;
            border: 2px solid #00aaaa;
            color: #00aaaa;
        }
        QPushButton#btnAutoMode:checked {
            background-color: #00aaaa;
            color: #000000;
        }

        /* 日志窗口 */
        QTextBrowser {
            background-color: #0a0a0a;
            color: #bbbbbb;
            border: none;
            font-family: "Consolas";
            font-size: 12px;
        }

        /* 状态标签 */
        QLabel {
            color: #ffffff;
        }
    )";
}

#endif // APPSTYLE_H
