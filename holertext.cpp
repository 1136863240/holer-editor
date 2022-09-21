#include "holertext.h"
#include <QDebug>
#include <cmath>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QApplication>
#include <QDir>

HolerText::HolerText(QWidget *parent) : QWidget(parent) {
    this->setFocusPolicy(Qt::WheelFocus);
    this->setAttribute(Qt::WA_InputMethodEnabled);

    this->clipboard = QApplication::clipboard();
    this->is_show_number = false;
    this->is_cursor_show = true;
    this->is_ctrl_down = false;
    this->is_shift_down = false;

    this->cursor_point.setX(0);
    this->cursor_point.setY(0);
    this->show_point.setX(0);
    this->show_point.setY(0);

    this->win_size.setWidth(0);
    this->win_size.setHeight(0);

    this->cursor_timer = new QTimer(this);
#if DEBUG == 0
    this->cursor_timer->setInterval(500);
    this->connect(this->cursor_timer, SIGNAL(timeout()),
                  this, SLOT(toggleCursorShow()));
#endif

    this->code_font = QFont("宋体", 12);
    this->canvas_color.setRgb(255, 255, 255);
    this->number_background_color.setRgb(255, 255, 255);
    this->number_color.setRgb(0, 0, 0);
    QFontMetrics fm(this->code_font);
    this->number_width = fm.width("1000000");

    this->cursor_color.setRgb(0, 0, 0);
    this->cursor_width = 1;

    this->lineMaxLength();

    this->need_auto_complete = false;
    this->open_auto_complete_win = false;
    this->auto_complete_font = QFont("宋体", 12);
    this->auto_complete_background_color.setRgb(255, 255, 255);
    this->auto_complete_color.setRgb(0, 0, 0);
    this->auto_complete_index = 0;
    this->auto_complete_index_start = 0;
    this->auto_complete_selected_background_color.setRgb(255, 0, 0);
    this->auto_complete_selected_font_color.setRgb(255, 255, 255);
    this->auto_complete_start = 0;
    this->auto_complete_length = 0;

    this->default_color.setRgb(255, 255, 255);

    this->selected_start_point.setX(-1);
    this->selected_start_point.setY(-1);
    this->selected_end_point.setX(-1);
    this->selected_end_point.setY(-1);

    QString keys = " `~!@#$%^&*()[]\\;',./{}|:\"<>?";
    int keys_length = keys.length();
    for (int i = 0; i < keys_length; ++i) {
        this->key_list.append(keys.at(i));
    }
}

HolerText::~HolerText() {
    this->cursor_timer->stop();
    delete this->cursor_timer;
}

void HolerText::paintEvent(QPaintEvent *) {
    QPainter p(this);

    QFontMetrics fm(this->code_font);
    int single_width = fm.width("0");
    int fm_height = fm.height();
    int show_x = this->show_point.x(), show_y = this->show_point.y();

    // 画总体的背景
    p.setPen(this->canvas_color);
    p.setBrush(this->canvas_color);
    p.drawRect(0, 0, this->win_size.width(), this->win_size.height());

    // 计算光标位置
    int cursor_x_px = 0, cursor_y_px = 0;
    int cursor_x = this->cursor_point.x(), cursor_y = this->cursor_point.y();
    int _cursor_x = 0, _cursor_y = -1;

    // 获取页面能放下的最大行数
    int max_line = this->maxLine();

    // 画行号
    if (this->is_show_number) {
        // 行号的背景
        p.setPen(this->number_background_color);
        p.setBrush(this->number_background_color);
        p.drawRect(0, 0, this->number_width, this->win_size.height());
        // 行号
        p.setPen(this->number_color);
        p.setBrush(this->number_color);
        p.setFont(this->code_font);
        for (int i = show_y, j = 1; i < max_line; ++i, ++j) {
            p.drawText(single_width, fm_height * j, QVariant(i + 1).toString());
        }
    }

    // 绘制文本
    int extra_width = single_width;
    if (this->is_show_number) {
        extra_width += this->number_width;
    }
    p.setFont(this->code_font);
    if (show_y < this->total_text.length()) {
        QString line = "";
        bool is_cursor = false;
        _cursor_y = show_y;
        // 如果start与end反了就交换一下
        QPoint start = this->selected_start_point, end = this->selected_end_point;
        if (start.y() > end.y() || (start.y() == end.y() && start.x() > end.x())) {
            QPoint temp = start;
            start = end;
            end = temp;
        }
        for (int i = show_y, j = 1, line_index = 0; i < max_line; ++i, ++j) {
            _cursor_x = show_x;
            if (_cursor_x == cursor_x && _cursor_y == cursor_y && !is_cursor) {
                cursor_x_px = 0;
                cursor_y_px = fm_height * line_index;
                is_cursor = true;
            }
            QList<QColor> color_list;
            if (this->total_rgb.length() <= i) {
                this->total_rgb.append(color_list);
            } else {
                color_list = this->total_rgb.at(i);
            }
            QVector<QString> str_vec = this->total_text.at(i);
            for (int str_i = show_x; str_i < str_vec.length(); ++str_i) {
                if (str_i >= str_vec.length()) {
                    break;
                }
                QString str = str_vec.at(str_i);
                if (_cursor_x == cursor_x && _cursor_y == cursor_y && !is_cursor) {
                    cursor_x_px = fm.width(line);
                    cursor_y_px = fm_height * line_index;
                    is_cursor = true;
                    QString log;
                    QTextStream ts(&log);
                    ts << "1: cursor_x:" << cursor_x
                        << " cursor_y:" << cursor_y
                        << " line:" << line;
                    this->log(__FUNCTION__, log);
                }
                QColor font_color = this->default_color;
                QColor background_color = this->canvas_color;
                int draw_x = extra_width + fm.width(line);
                if (color_list.length() > str_i) {
                    font_color = color_list.at(str_i);
                }
                if (this->isSelected(start, end, _cursor_x, _cursor_y)) {
                    font_color = this->selected_color;
                    background_color = this->selected_background_color;
                    p.setPen(background_color);
                    p.setBrush(background_color);
                    p.drawRect(draw_x, fm_height * (j - 1), fm.width(str), fm_height);
                }
                p.setPen(font_color);
                p.setBrush(font_color);
                p.drawText(draw_x, fm_height * j, str);
                line += str;
                ++_cursor_x;
                if (_cursor_x == cursor_x && _cursor_y == cursor_y && !is_cursor) {
                    cursor_x_px = fm.width(line);
                    cursor_y_px = fm_height * line_index;
                    is_cursor = true;
                    QString log;
                    QTextStream ts(&log);
                    ts << "2: cursor_x:" << cursor_x
                       << " cursor_y:" << cursor_y
                       << " line:" << line;
                    this->log(__FUNCTION__, log);
                }
            }
            line = "";
            ++_cursor_y;
            ++line_index;
        }
    }

    // 画光标
    if (this->is_cursor_show) {
        p.setPen(this->cursor_color);
        p.setBrush(this->cursor_color);
    } else {
        p.setPen(this->canvas_color);
        p.setBrush(this->canvas_color);
    }
    QString log;
    QTextStream ts(&log);
    ts << "cursor_x:" << cursor_x
       << " cursor_y:" <<  cursor_y
       << " _cursor_x:" << _cursor_x
       << " show_x:" << show_x
       << " show_y:" << show_y;
    this->log(__FUNCTION__, log);
    p.drawRect(extra_width + cursor_x_px,
               cursor_y_px, this->cursor_width, fm_height);

    // 画自动补全窗口
    if (this->need_auto_complete && this->open_auto_complete_win) {
        QFontMetrics fm_auto(this->auto_complete_font);
        int fm_auto_height = fm_auto.height();
        int max_item = 0;
        int auto_index = this->auto_complete_index;
        int index_start = this->auto_complete_index_start;
        int text_list_length = this->auto_complete_text_all.length();
        if (text_list_length > 4) {
            max_item = 4;
        } else {
            max_item = text_list_length;
        }
        int auto_max_width_px = 0;
        QString status_text =
                QVariant(index_start + auto_index + 1).toString() + "/" +
                QVariant(text_list_length).toString();
        if (status_text.length() > this->auto_complete_max_string.length()) {
            auto_max_width_px = fm_auto.width(status_text);
        } else {
            auto_max_width_px = fm_auto.width(this->auto_complete_max_string);
        }
        int auto_max_height_px = fm_auto_height * (max_item + 1);
        // 计算并画边框
        int win_width = this->win_size.width(), win_height = this->win_size.height();
        int frame_x = 0, frame_y = 0;
        if (auto_max_width_px + cursor_x_px > win_width) {
            frame_x = win_width - auto_max_width_px;
            if (frame_x < 0) {
                frame_x = 0;
            }
        } else {
            frame_x = this->number_width + cursor_x_px;
        }
        if (cursor_y_px + auto_max_height_px + fm_height > win_height) {
            frame_y = win_height - auto_max_height_px;
            if (frame_y < 0) {
                frame_y = 0;
            }
        } else {
            frame_y = cursor_y_px + fm_auto_height;
        }
        int auto_width = single_width * 2 + auto_max_width_px;
        p.setFont(this->auto_complete_font);
        p.setPen(this->auto_complete_background_color);
        p.setBrush(this->auto_complete_background_color);
        p.drawRect(frame_x, frame_y, auto_width, (max_item + 1) * fm_auto_height);
        // 画自动补全提示栏
        p.setPen(this->auto_complete_status_background_color);
        p.setBrush(this->auto_complete_status_background_color);
        p.drawRect(frame_x, frame_y, auto_width, fm_auto_height);
        p.setPen(this->auto_complete_status_color);
        p.setBrush(this->auto_complete_status_color);
        p.drawText(single_width + frame_x, frame_y + fm_auto_height, status_text);
        // 画元素
        int i = 1;
        for (QString auto_str : this->auto_complete_text_list) {
            int item_x = frame_x, item_y = frame_y + (fm_auto_height * (i + 1));
            if (i == auto_index + 1) {
                // 画已选中的元素背景
                p.setPen(this->auto_complete_selected_background_color);
                p.setBrush(this->auto_complete_selected_background_color);
                p.drawRect(item_x, frame_y + (fm_auto_height * i),
                           single_width * 2 + fm_auto.width(this->auto_complete_max_string),
                           fm_auto_height);
                // 画已选中的元素文字
                p.setPen(this->auto_complete_selected_font_color);
                p.setBrush(this->auto_complete_selected_font_color);
                p.drawText(single_width + item_x, item_y, auto_str);
            } else {
                // 画未选中的文字
                p.setPen(this->auto_complete_color);
                p.setPen(this->auto_complete_color);
                p.drawText(single_width + item_x, item_y, auto_str);
            }
            ++i;
        }
    }
}

void HolerText::resizeEvent(QResizeEvent *event) {
    this->win_size = event->size();
    this->lineMaxLength();
}

void HolerText::showEvent(QShowEvent *) {
    this->cursor_timer->start();
}

void HolerText::hideEvent(QHideEvent *) {
    this->cursor_timer->stop();
}

QPoint HolerText::mousePoint(QPoint xy) {
    QPoint pos = xy, res;
    QFontMetrics fm(this->code_font);
    int single_width = fm.width("0");
    int fm_height = fm.height();
    int show_x = this->show_point.x(), show_y = this->show_point.y();
    int mouse_x = pos.x() - this->number_width - single_width, mouse_y = pos.y();
    int mouse_line_y = (int)floor(mouse_y / fm_height);
    int max_line = this->total_text.length() - show_y - 1;
    if (mouse_line_y > max_line) {
        mouse_line_y = max_line;
    }
    res.setY(mouse_line_y);
    QString mouse_line_str = this->line(mouse_line_y);
    int mouse_line_x = 0;
    int mouse_line_x_px = 0;
    int mouse_line_length = mouse_line_str.length();
    QString line_temp = "";
    for (int i = show_x; i < mouse_line_length; ++i, ++mouse_line_x) {
        QChar c = mouse_line_str.at(i);
        int c_px = fm.width(c);
        int half_c_px = c_px / 2;
        if (mouse_line_x_px + half_c_px >= mouse_x) {
            break;
        }
        line_temp += c;
        mouse_line_x_px = fm.width(line_temp);
    }
    res.setX(mouse_line_x);
    return res;
}

void HolerText::mousePressEvent(QMouseEvent *event) {
    Qt::MouseButton mouse_button = event->button();
    if (mouse_button == Qt::LeftButton) {
        this->mouse_left_down = true;
        this->cursor_point = this->mousePoint(event->pos());
        this->selected_start_point = this->cursor_point;
        this->selected_end_point = this->cursor_point;
        this->open_auto_complete_win = false;
    }
    this->is_cursor_show = true;
    this->update();
}

void HolerText::mouseMoveEvent(QMouseEvent *event) {
    if (this->mouse_left_down) {
        QPoint pos = event->pos();
        if (pos.y() < 0) {
            pos.setY(0);
        }
        this->cursor_point = this->mousePoint(pos);
        this->selected_end_point = this->cursor_point;
        this->update();
    }
}

void HolerText::mouseReleaseEvent(QMouseEvent *) {
    if (this->mouse_left_down) {
        this->mouse_left_down = false;
        this->update();
    }
}

bool HolerText::isSelected(QPoint start, QPoint end, int x, int y) {
    int start_x = start.x(), start_y = start.y(), end_x = end.x(), end_y = end.y();
    // 是否需要选择文本
    bool need_selected = false;
    if (start_x != end_x || start_y != end_y) {
        need_selected = true;
    }
    // 判断xy是否在选择范围中
    if (need_selected) {
        bool in_same_line = (start_y == end_y);
        if (in_same_line) {
            return (x >= start_x && x <= end_x && y >= start_y && y <= end_y);
        } else {
            if (y > start_y && y < end_y) {
                return true;
            } else if (y == start_y) {
                return x >= start_x;
            } else if (y == end_y) {
                return x < end_x;
            }
        }
    }
    return false;
}

void HolerText::setShowNumber(bool is_show_number) {
    this->is_show_number = is_show_number;
}

void HolerText::setCodeFont(QFont font) {
    this->code_font = font;
    this->lineMaxLength();
}

void HolerText::setCursorPoint(int x, int y) {
    this->cursor_point.setX(x);
    this->cursor_point.setY(y);
    this->selected_start_point = this->cursor_point;
    this->selected_end_point = this->cursor_point;
}

void HolerText::setShowPoint(int x, int y) {
    this->show_point.setX(x);
    this->show_point.setY(y);
}

void HolerText::setCursorTwinkleTime(int msec) {
    this->cursor_timer->setInterval(msec);
    if (this->cursor_timer->isActive()) {
        this->cursor_timer->stop();
        this->cursor_timer->start();
    }
}

void HolerText::setCanvasSize(int x, int y, QSize size) {
    this->win_size = size;
    this->lineMaxLength();
    this->setGeometry(x, y, size.width(), size.height());
}

void HolerText::setCanvasColor(QColor color) {
    this->canvas_color = color;
}

void HolerText::setNumberBackgroundColor(QColor color) {
    this->number_background_color = color;
}

void HolerText::setNumberColor(QColor color) {
    this->number_color = color;
}

void HolerText::toggleCursorShow() {
    this->is_cursor_show = !this->is_cursor_show;
    this->update();
}

void HolerText::setNumberWidth(int width) {
    this->number_width = width;
}

void HolerText::setText(QString text) {
    this->total_text.clear();
    this->total_rgb.clear();
    QStringList str_list = text.split("\n");
    QVector<QString> line;
    QList<QColor> line_color;
    for (QString str : str_list) {
        if (str.length() == 0) {
            line.clear();
            line_color.clear();
            this->total_rgb.append(line_color);
            this->total_text.append(line);
        } else {
            for (int i = 0; i < str.length(); ++i) {
                QChar c = str.at(i);
                QString s(c);
                line.append(s);
                line_color.append(QColor(255, 255, 255));
            }
            if (line.length() > 0) {
                this->total_text.append(line);
                line.clear();
            }
            if (line_color.length() > 0) {
                this->total_rgb.append(line_color);
                line_color.clear();
            }
        }
    }
}

void HolerText::setCursorColor(QColor color) {
    this->cursor_color = color;
}

void HolerText::setCursorWidth(int width) {
    this->cursor_width = width;
}

void HolerText::setAutoComplete(bool need) {
    this->need_auto_complete = need;
}

void HolerText::setAutoCompleteBackgroundColor(QColor color) {
    this->auto_complete_background_color = color;
}

void HolerText::setAutoCompletePlugin(QString plugin_name) {
    this->auto_complete_plugin = plugin_name;
    if (this->plugins.contains(plugin_name)) {
        QJSEngine engine;
        QJSValue exec = engine.evaluate("(" + this->plugins[plugin_name] + ")");
        if (exec.isError()) {
            QMessageBox::critical(
                        this, "系统错误", plugin_name + "插件有语法错误，出错点：" +
                        exec.property("lineNumber").toString() +
                        "\n错误信息：" + exec.toString());
            QString log;
            QTextStream ts(&log);
            ts << "line:" << exec.property("lineNumber").toInt()
               << " error info:" << exec.toString().toUtf8();
            this->log(__FUNCTION__, log);
        }
    } else {
        QMessageBox::critical(this, "系统错误", plugin_name + "插件不存在");
    }
}

void HolerText::setAutoCompleteColor(QColor color) {
    this->auto_complete_color = color;
}

void HolerText::setDefaultColor(QColor color) {
    this->default_color = color;
}

void HolerText::keyPressEvent(QKeyEvent *event) {
    int key = event->key();
    QString text = event->text();
#if DEBUG == 1
    qDebug() << "key:" << key
             << " text:" << text;
#endif

    QFontMetrics fm(this->code_font);
    int fm_height = fm.height();
    int cursor_x = this->cursor_point.x();
    int cursor_y = this->cursor_point.y();
    int show_x = this->show_point.x();
    int show_y = this->show_point.y();
    int max_y = (int)floor(this->win_size.height() / fm_height);
    int max_line = this->total_text.length();
    int _max_y = max_y + show_y;
    if (max_line > _max_y) {
        max_line = _max_y;
    }
    QMap<QString, QVariant> res;
    bool is_flush = false;
    if (text == "") {
        int all_length = this->auto_complete_text_all.length();
        int list_length = this->auto_complete_text_list.length();
        int all_page = (int)ceil(all_length / 4);
        int index = this->auto_complete_index;
        int index_start = this->auto_complete_index_start;
        int current_page = (int)ceil(index_start / 4);
        if (key == Qt::Key_Up) {
            if (this->need_auto_complete && this->open_auto_complete_win) {
                if (index > 0) {
                    --this->auto_complete_index;
                }
            } else {
                res = this->eval_int(key);
                is_flush = true;
            }
        } else if (key == Qt::Key_Down) {
            if (this->need_auto_complete && this->open_auto_complete_win) {
                if (index >= list_length - 1) {
                    if (current_page == all_page) {
                        this->auto_complete_index_start = 0;
                        this->auto_complete_index = 0;
                        this->auto_complete_text_list.clear();
                        for (int i = 0; i < 4; ++i) {
                            this->auto_complete_text_list.append(this->auto_complete_text_all.at(i));
                        }
                    } else if (current_page < all_page) {
                        ++current_page;
                        this->auto_complete_text_list.clear();
                        int leftover = all_length - current_page * 4;
                        leftover = leftover > 4 ? 4 : leftover;
                        for (int i = 0; i < leftover; ++i) {
                            this->auto_complete_text_list.append(
                                        this->auto_complete_text_all.at(current_page * 4 + i));
                        }
                        this->auto_complete_index_start += 4;
                    }
                    this->auto_complete_index = 0;
                } else {
                    ++this->auto_complete_index;
                }
            } else {
                res = this->eval_int(key);
                is_flush = true;
            }
        } else if (key == Qt::Key_Left) {
            res = this->eval_int(key);
            is_flush = true;
        } else if (key == Qt::Key_Right) {
            res = this->eval_int(key);
            is_flush = true;
        } else if (key == Qt::Key_Control) {
            this->is_ctrl_down = true;
        } else if (key == Qt::Key_Shift) {
            this->is_shift_down = true;
        } else if (key == Qt::Key_Home) {
            if (this->is_ctrl_down) {
                this->cursor_point.setY(0);
                this->show_point.setY(0);
            }
            this->cursor_point.setX(0);
            this->show_point.setX(0);
            this->open_auto_complete_win = false;
        } else if (key == Qt::Key_End) {
            res = this->eval_int(key);
            is_flush = true;
            this->open_auto_complete_win = false;
        }
        if (is_flush) {
            this->cursor_point.setX(res["cursor_x"].toInt());
            this->cursor_point.setY(res["cursor_y"].toInt());
            this->show_point.setX(res["show_x"].toInt());
            this->show_point.setY(res["show_y"].toInt());
            if (this->is_shift_down) {
                this->selected_end_point = this->cursor_point;
            } else {
                this->selected_start_point = this->cursor_point;
                this->selected_end_point = this->cursor_point;
            }
        }
    } else {
        bool is_select = false;
        if (text == "\b") {
            if (this->open_auto_complete_win) {
                this->open_auto_complete_win = false;
            }
            res = this->eval_qstring(text);
            is_flush = true;
        } else if (text == "\r") {
            if (this->open_auto_complete_win) {
                this->open_auto_complete_win = false;
            }
            res = this->eval_qstring(text);
            is_flush = true;
        } else if (text == "\t") {
            if (this->open_auto_complete_win) {
                int length = this->auto_complete_length + this->auto_complete_start;
                for (int i = this->auto_complete_start; i < length; ++i) {
                    this->total_text[cursor_y].remove(i);
                }
                QString submit = this->auto_complete_text_list.at(this->auto_complete_index);
                int submit_length = submit.length();
                for (int i = submit_length - 1; i >= 0; --i) {
                    this->total_text[cursor_y].insert(this->auto_complete_start, submit.at(i));
                }
                this->cursor_point.setX(cursor_x + submit_length - 1);
                this->open_auto_complete_win = false;
            } else {
                int max_length = this->max_line_length;
                this->total_text[cursor_y].insert(cursor_x, "    ");
                ++cursor_x;
                this->cursor_point.setX(cursor_x);
                while (cursor_x > max_length + show_x) {
                    ++show_x;
                    this->show_point.setX(show_x);
                }
                this->cursor_point.setX(cursor_x);
            }
        } else if (text == "\u0003") { // Ctrl + C
            if (this->is_ctrl_down) {
                res = this->eval_qstring(text);
                this->clipboard->setText(res["copy_str"].toString());
            }
        } else if (text == "\u0016") { // Ctrl + V
            if (this->is_ctrl_down) {
                QString str = this->clipboard->text();
                res = this->eval_qstring(text, str);
                is_flush = true;
            }
        } else if (text == "\u007F") {  // Delete
            res = this->eval_qstring(text);
            is_flush = true;
        } else if (text == "\u0001") { // Ctrl + A
            int text_length = this->total_text.length();
            int last_line_length = this->total_text.at(text_length - 1).length();
            this->selected_start_point.setX(0);
            this->selected_start_point.setY(0);
            this->selected_end_point.setX(last_line_length);
            this->selected_end_point.setY(text_length - 1);
            this->cursor_point = this->selected_end_point;
            is_select = true;
        } else if ((key >= Qt::Key_0 && key <= Qt::Key_9) ||
                   (key >= Qt::Key_A && key <= Qt::Key_Z) ||
                   this->key_list.contains(text)) {
            int max_length = this->max_line_length;
            this->total_text[cursor_y].insert(cursor_x, text);
            ++cursor_x;
            this->cursor_point.setX(cursor_x);
            while (cursor_x > max_length + show_x) {
                ++show_x;
            }
            this->show_point.setX(show_x);
            this->execAutoComplete(this->line(this->cursor_point.y()));
        }
        if (is_flush) {
            this->setText(res["text"].toString());
            this->cursor_point.setX(res["cursor_x"].toInt());
            this->cursor_point.setY(res["cursor_y"].toInt());
            this->show_point.setX(res["show_x"].toInt());
            this->show_point.setY(res["show_y"].toInt());
        }
        if (!is_select) {
            this->selected_start_point = this->cursor_point;
            this->selected_end_point = this->cursor_point;
        }
        this->setCodeColorPlugin(this->code_color_plugin);
    }
    QString log;
    QTextStream ts(&log);
    ts << "cursor_x:" << this->cursor_point.x()
       << " cursor_y:" << this->cursor_point.y()
       << " show_x:" << this->show_point.x()
       << " show_y:" << this->show_point.y()
       << " start_select_x:" << this->selected_start_point.x()
       << " start_select_y:" << this->selected_start_point.y()
       << " end_select_x:" << this->selected_end_point.x()
       << " end_select_y:" << this->selected_end_point.y();
    this->log(__FUNCTION__, log);
    this->is_cursor_show = true;
    this->update();
}

QMap<QString, QVariant> HolerText::eval_int(int key) {
    QMap<QString, QVariant> empty;
    if (this->key_plugins_int.contains(key)) {
        QJSEngine engine;
        QJSValue value;
        QString script = this->plugins[this->key_plugins_int[key]];
        value = engine.evaluate("(" + script + ")");
        if (value.isError()) {
            QString log;
            QTextStream ts(&log);
            ts << "lineNumber: " << value.property("lineNumber").toInt()
               << " error info: " << value.toString();
            this->log(__FUNCTION__, log);
            QMessageBox::critical(this, "脚本错误", "插件: " + this->key_plugins_int[key] + "\n" + log);
        } else {
            QJSValueList v_list;
            QFontMetrics fm(this->code_font);
            int fm_height = fm.height();
            int cursor_x = this->cursor_point.x();
            int cursor_y = this->cursor_point.y();
            int show_x = this->show_point.x();
            int show_y = this->show_point.y();
            int max_y = (int)floor(this->win_size.height() / fm_height);
            int max_line = this->total_text.length();
            int _max_y = max_y + show_y;
            if (max_line > _max_y) {
                max_line = _max_y;
            }
            v_list << this->allText()
                   << cursor_x
                   << cursor_y
                   << show_x
                   << show_y
                   << max_y
                   << max_line
                   << this->max_line_length
                   << this->is_ctrl_down
                   << this->is_shift_down;
            QJSValue result = value.call(v_list);
            QMap<QString, QVariant> res = result.toVariant().toMap();
            QString log;
            QTextStream ts(&log);
            ts << "cursor_x: " << res["cursor_x"].toInt()
               << " cursor_y: " << res["cursor_y"].toInt()
               << " show_x: " << res["show_x"].toInt()
               << " show_y: " << res["show_y"].toInt();
            this->log(__FUNCTION__, log);
            return res;
        }
    }
    return empty;
}

QMap<QString, QVariant> HolerText::eval_qstring(QString key, QString extra_str) {
    QMap<QString, QVariant> empty;
    if (this->key_plugins_qstring.contains(key)) {
        QJSEngine engine;
        QJSValue value;
        QString script = this->plugins[this->key_plugins_qstring[key]];
        value = engine.evaluate("(" + script + ")");
        if (value.isError()) {
            QString log;
            QTextStream ts(&log);
            ts << "lineNumber: " << value.property("lineNumber").toInt()
               << " error info: " << value.toString();
            this->log(__FUNCTION__, log);
            QMessageBox::critical(this, "脚本错误", "插件: " + this->key_plugins_qstring[key] + "\n" + log);
        } else {
            QJSValueList v_list;
            QFontMetrics fm(this->code_font);
            int fm_height = fm.height();
            int cursor_x = this->cursor_point.x();
            int cursor_y = this->cursor_point.y();
            int show_x = this->show_point.x();
            int show_y = this->show_point.y();
            int max_y = (int)floor(this->win_size.height() / fm_height);
            int max_line = this->total_text.length();
            int _max_y = max_y + show_y;
            if (max_line > _max_y) {
                max_line = _max_y;
            }
            v_list << this->allText()
                   << cursor_x
                   << cursor_y
                   << show_x
                   << show_y
                   << max_y
                   << max_line
                   << extra_str
                   << this->max_line_length
                   << this->is_ctrl_down
                   << this->is_shift_down
                   << this->selected_start_point.x()
                   << this->selected_start_point.y()
                   << this->selected_end_point.x()
                   << this->selected_end_point.y();
            QJSValue result = value.call(v_list);
            QMap<QString, QVariant> res = result.toVariant().toMap();
            QString log;
            QTextStream ts(&log);
            ts << "cursor_x: " << res["cursor_x"].toInt()
               << " cursor_y: " << res["cursor_y"].toInt()
               << " show_x: " << res["show_x"].toInt()
               << " show_y: " << res["show_y"].toInt();
            this->log(__FUNCTION__, log);
            return res;
        }
    }
    return empty;
}

void HolerText::keyReleaseEvent(QKeyEvent *event) {
    int key = event->key();
    if (key == Qt::Key_Control) {
        this->is_ctrl_down = false;
    }
    if (key == Qt::Key_Shift) {
        this->is_shift_down = false;
    }
}

void HolerText::inputMethodEvent(QInputMethodEvent *e) {
    int cursor_x = this->cursor_point.x(),
            cursor_y = this->cursor_point.y();
    QString text = e->commitString();
    int text_length = text.length();
    for (int i = 0; i < text_length; ++i) {
        this->total_text[cursor_y].insert(cursor_x, text.at(i));
    }
    this->cursor_point.setX(cursor_x + text_length);
    this->is_cursor_show = true;
    this->execAutoComplete(this->line(this->cursor_point.y()));
    this->setCodeColorPlugin(this->code_color_plugin);
    this->update();
}

void HolerText::wheelEvent(QWheelEvent *event) {
    bool is_down = event->delta() > 0 ? false : true;
    QFontMetrics fm(this->code_font);
    int fm_height = fm.height();
    int max_y = (int)floor(this->win_size.height() / fm_height);
    int max_line = this->maxLineMinus();
    int cursor_x = this->cursor_point.x();
    int cursor_y = this->cursor_point.y();
    int show_x = this->show_point.x();
    int show_y = this->show_point.y();
    if (is_down) {
        cursor_y += 3;
        if (cursor_y >= max_line) {
            cursor_y = max_line;
        }
        if (cursor_y < this->total_text.length()) {
            this->cursor_point.setY(cursor_y);
            int line_length = this->line(cursor_y).length();
            if (cursor_x > line_length) {
                cursor_x = line_length;
            }
            this->cursor_point.setX(cursor_x);
            while (cursor_x <= show_x && show_x > 0) {
                --show_x;
            }
            this->show_point.setX(show_x);
            while (cursor_y + 1 > max_y + show_y && show_y < max_y - 1) {
                ++show_y;
            }
            this->show_point.setY(show_y);
        }
    } else {
        cursor_y -= 3;
        if (cursor_y < 0) {
            cursor_y = 0;
        }
        if (cursor_y <= show_y) {
            this->show_point.setY(cursor_y);
        }
        this->cursor_point.setY(cursor_y);
        int prev_x = this->total_text.at(cursor_y).length();
        if (cursor_x > prev_x) {
            cursor_x = prev_x;
        }
        this->cursor_point.setX(cursor_x);
        while (cursor_x <= show_x && show_x > 0) {
            --show_x;
        }
        this->show_point.setX(show_x);
    }
    this->update();
}

QString HolerText::line(int line_number) {
    QString l = "";
    if (line_number < 0 || line_number >= this->total_text.length()) {
        return l;
    }
    for (QString str : this->total_text.at(line_number)) {
        l += str;
    }
    return l;
}

void HolerText::lineMaxLength() {
    int max_length = 1;
    QFontMetrics fm(this->code_font);
    int win_width = this->win_size.width() - this->number_width;
    QString str = "我";
    while (fm.width(str) < win_width) {
        str += "我";
        ++max_length;
    }
    this->max_line_length = max_length;
}

int HolerText::maxLine() {
    // 计算窗口能放下的最大行数
    QFontMetrics fm(this->code_font);
    int fm_height = fm.height();
    int show_y = this->show_point.y();
    int max_y = (int)ceil(this->win_size.height() / fm_height);
    int max_line = this->total_text.length();
    int _max_y = max_y + show_y;
    if (max_line > _max_y) {
        max_line = _max_y;
    }
    return max_line;
}

int HolerText::maxLineMinus() {
    // 计算除了多余行的行数
    QFontMetrics fm(this->code_font);
    int fm_height = fm.height();
    int show_y = this->show_point.y();
    int max_y = (int)floor(this->win_size.height() / fm_height);
    int max_line = this->total_text.length();
    int _max_y = max_y + show_y;
    if (max_line > _max_y) {
        max_line = _max_y;
    }
    return max_line;
}

void HolerText::execAutoComplete(QString current_line) {
    QString plugin_name = this->auto_complete_plugin;
    if (this->plugins.contains(plugin_name)) {
        QJSValueList param_list;
        param_list << this->allText() << current_line << this->cursor_point.x();
        QJSEngine engine;
        QJSValue exec = engine.evaluate("(" + this->plugins[plugin_name] + ")");
        if (exec.isError()) {
            QMessageBox::critical(
                        this, "系统错误", plugin_name + "插件有语法错误，出错点：" +
                        exec.property("lineNumber").toString() +
                        "\n错误信息：" + exec.toString());
            QString log;
            QTextStream ts(&log);
            ts << "line:" << exec.property("lineNumber").toInt()
               << "error info:" << exec.toString().toUtf8();
            this->log(__FUNCTION__, log);
            return;
        }
        QList<QVariant> res = exec.call(param_list).toVariant().toList();
        if (res.length() != 5) {
            QMessageBox::critical(this, "系统错误", plugin_name + "插件的返回值元素数量必须等于5");
            return;
        }
        this->open_auto_complete_win = res.at(0).toBool();
        this->auto_complete_max_string = res.at(1).toString();
        this->auto_complete_start = res.at(2).toInt();
        this->auto_complete_length = res.at(3).toInt();
        QList<QVariant> text_list = res.at(4).toList();
        this->auto_complete_text_all.clear();
        for (QVariant item : text_list) {
            this->auto_complete_text_all.append(item.toString());
        }
        int all_length = this->auto_complete_text_all.length();
        int max_auto_length = all_length > 4 ? 4 : all_length;
        this->auto_complete_text_list.clear();
        for (int i = 0; i < max_auto_length; ++i) {
            this->auto_complete_text_list.append(this->auto_complete_text_all.at(i));
        }
        this->auto_complete_index = 0;
        this->auto_complete_index_start = 0;
    }
}

QString HolerText::allText() {
    QString text = "";
    QStringList lines;
    for (QVector<QString> str_vec : this->total_text) {
        for (QString str : str_vec) {
            text += str;
        }
        lines.append(text);
        text.clear();
    }
    return lines.join("\n");
}

bool HolerText::hasPlugin(QString plugin_name) {
    return this->plugins.contains(plugin_name);
}

bool HolerText::addPluginFile(QString file_name, QString plugin_name) {
    QFile file(file_name);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        QString log;
        QTextStream ts(&log);
        ts << "cannot open file:" << file_name.toUtf8();
        this->log(__FUNCTION__, log);
        return false;
    }
    QTextStream ts(&file);
    QString js_code = ts.readAll();
    file.close();
    if (this->plugins.contains(plugin_name)) {
        this->plugins[plugin_name] = js_code;
    } else {
        this->plugins.insert(plugin_name, js_code);
    }
    return true;
}

void HolerText::removePlugin(QString plugin_name) {
    if (this->plugins.contains(plugin_name)) {
        this->plugins.remove(plugin_name);
    }
}

void HolerText::setKeyPluginInt(int key, QString plugin_name) {
    if (this->key_plugins_int.contains(key)) {
        this->key_plugins_int[key] = plugin_name;
    } else {
        this->key_plugins_int.insert(key, plugin_name);
    }
}

void HolerText::setKeyPluginQString(QString key, QString plugin_name) {
    if (this->key_plugins_qstring.contains(key)) {
        this->key_plugins_qstring[key] = plugin_name;
    } else {
        this->key_plugins_qstring.insert(key, plugin_name);
    }
}

void HolerText::setCodeColorPlugin(QString plugin_name) {
    this->code_color_plugin = plugin_name;
    if (this->plugins.contains(plugin_name)) {
        QJSValueList param_list;
        param_list << this->allText();
        QJSEngine engine;
        QJSValue exec = engine.evaluate("(" + this->plugins[plugin_name] + ")");
        if (exec.isError()) {
            QMessageBox::critical(
                        this, "系统错误", plugin_name + "插件有语法错误，出错点：" +
                        exec.property("lineNumber").toString() +
                        "\n错误信息：" + exec.toString());
            QString log;
            QTextStream ts(&log);
            ts << "line:" << exec.property("lineNumber").toInt()
               << "error info:" << exec.toString().toUtf8();
            this->log(__FUNCTION__, log);
            return;
        }
        QJSValue v = exec.call(param_list);
        QList<QVariant> list1 = v.toVariant().toList();
        this->total_rgb.clear();
        for (QVariant d : list1) {
            QList<QColor> color_list;
            QList<QVariant> list2 = d.toList();
            for (QVariant d2 : list2) {
                QColor color;
                QList<QVariant> list3 = d2.toList();
                if (list3.length() == 3) {
                    color.setRgb(list3.at(0).toInt(),
                                 list3.at(1).toInt(),
                                 list3.at(2).toInt());
                } else if (list3.length() == 4) {
                    color.setRgb(list3.at(0).toInt(),
                                 list3.at(1).toInt(),
                                 list3.at(2).toInt(),
                                 list3.at(3).toInt());
                }
                color_list.append(color);
            }
            this->total_rgb.append(color_list);
        }

        // 如果数组不够则用默认色填充
        int text_length = this->total_text.length();
        for (int i = 0; i < text_length; ++i) {
            QList<QColor> color_list;
            if (this->total_rgb.length() <= i) {
                this->total_rgb.append(color_list);
            }
            int sub_text_length = this->total_text.at(i).length();
            for (int j = 0; j < sub_text_length; ++j) {
                if (this->total_rgb.at(i).length() <= j) {
                    this->total_rgb[i].append(this->default_color);
                }
            }
        }
    }
}

void HolerText::setAutoCompleteSelectedBackgroundColor(QColor color) {
    this->auto_complete_selected_background_color = color;
}

void HolerText::setAutoCompleteSelectedFontColor(QColor color) {
    this->auto_complete_selected_font_color = color;
}

void HolerText::setAutoCompleteStatusBackground_color(QColor color) {
    this->auto_complete_status_background_color = color;
}

void HolerText::setAutoCompleteStatusColor(QColor color) {
    this->auto_complete_status_color = color;
}

void HolerText::setSelectedColor(QColor color) {
    this->selected_color = color;
}

void HolerText::setSelectedBackgroundColor(QColor color) {
    this->selected_background_color = color;
}

QVector<QString> HolerText::line2vec(QString str) {
    QVector<QString> vec;
    int str_length = str.length();
    for (int i = 0; i < str_length; ++i) {
        vec.append(str.at(i));
    }
    return vec;
}

QPoint HolerText::cursorPoint() {
    return this->cursor_point;
}

QPoint HolerText::showPoint() {
    return this->show_point;
}

void HolerText::log(QString func_name, QString log) {
    Q_UNUSED(func_name);
    Q_UNUSED(log);
#if DEBUG == 1
    if (log.length() > 0) {
        qDebug() << "HolerText:" << func_name << ":" << endl
                 << log << endl
                 << "--------------------------------------------" << endl;
        QFile log_file("debug.log");
        if (log_file.open(QFile::WriteOnly | QFile::Append | QFile::Text)) {
            QTextStream ts(&log_file);
            ts << "HolerText:" << func_name << ":" << endl
               << log << endl
               << "--------------------------------------------" << endl;
            log_file.close();
        }
    }
#endif
}
