#ifndef HOLERTEXT_H
#define HOLERTEXT_H

#include <QWidget>
#include <QPainter>
#include <QFont>
#include <QTimer>
#include <QPoint>
#include <QResizeEvent>
#include <QColor>
#include <QFontMetrics>
#include <QString>
#include <QVector>
#include <QList>
#include <QKeyEvent>
#include <QInputMethodEvent>
#include <QMouseEvent>
#include <QMap>
#include <QTextStream>
#include <QJSEngine>
#include <QJSValue>
#include <QClipboard>
#include <QWheelEvent>

#define DEBUG 1

class HolerText : public QWidget
{
    Q_OBJECT

    QClipboard *clipboard;
    bool is_show_number;
    bool is_cursor_show;
    bool need_auto_complete;
    bool open_auto_complete_win;
    bool mouse_left_down;
    bool is_ctrl_down;
    bool is_shift_down;
    QPoint cursor_point;
    QPoint show_point;
    QPoint selected_start_point;
    QPoint selected_end_point;
    QSize win_size;
    QFont code_font;
    QFont auto_complete_font;
    QTimer *cursor_timer;
    QColor canvas_color;
    QColor number_background_color;
    QColor number_color;
    QColor cursor_color;
    QColor auto_complete_color;
    QColor auto_complete_background_color;
    QColor auto_complete_selected_background_color;
    QColor auto_complete_selected_font_color;
    QColor auto_complete_status_background_color;
    QColor auto_complete_status_color;
    QColor default_color;
    QColor selected_color;
    QColor selected_background_color;
    int number_width;
    int cursor_width;
    int max_line_length;
    int auto_complete_index;
    int auto_complete_index_start;
    int auto_complete_start;
    int auto_complete_length;
    QVector<QVector<QString>> total_text;
    QList<QList<QColor>> total_rgb;
    QList<QString> auto_complete_text_list;
    QList<QString> auto_complete_text_all;
    QList<QString> key_list;
    QMap<QString, QString> plugins;
    QMap<int, QString> key_plugins_int;
    QMap<QString, QString> key_plugins_qstring;
    QString code_color_plugin;
    QString auto_complete_plugin;
    QString auto_complete_max_string;
    void lineMaxLength();
    int maxLine();
    int maxLineMinus();
    void execAutoComplete(QString current_line);
    QPoint mousePoint(QPoint xy);
    bool isSelected(QPoint start, QPoint end, int x, int y);
    QVector<QString> line2vec(QString str);
    QMap<QString, QVariant> eval_int(int key);
    QMap<QString, QVariant> eval_qstring(QString key, QString extra_str = "");

    void log(QString func_name, QString log = "");

public:
    explicit HolerText(QWidget *parent = 0);
    ~HolerText();

    void setShowNumber(bool is_show_number);
    void setCodeFont(QFont font);
    void setCursorPoint(int x, int y);
    void setShowPoint(int x, int y);
    void setCursorTwinkleTime(int msec);
    void setCanvasSize(int x, int y, QSize size);
    void setCanvasColor(QColor color);
    void setNumberBackgroundColor(QColor color);
    void setNumberColor(QColor color);
    void setNumberWidth(int width);
    void setText(QString text);
    void setCursorColor(QColor color);
    void setCursorWidth(int width);
    void setCodeColorPlugin(QString plugin_name);
    void setAutoComplete(bool need);
    void setAutoCompletePlugin(QString plugin_name);
    void setAutoCompleteBackgroundColor(QColor color);
    void setAutoCompleteColor(QColor color);
    void setDefaultColor(QColor color);
    void setAutoCompleteSelectedBackgroundColor(QColor color);
    void setAutoCompleteSelectedFontColor(QColor color);
    void setAutoCompleteStatusBackground_color(QColor color);
    void setAutoCompleteStatusColor(QColor color);
    void setSelectedColor(QColor color);
    void setSelectedBackgroundColor(QColor color);

    QString line(int line_number);
    QString allText();
    bool hasPlugin(QString plugin_name);
    bool addPluginFile(QString file_name, QString plugin_name);
    void removePlugin(QString plugin_name);

    void setKeyPluginInt(int key, QString plugin_name);
    void setKeyPluginQString(QString key, QString plugin_name);

    QPoint cursorPoint();
    QPoint showPoint();

protected:
    void paintEvent(QPaintEvent *);
    void resizeEvent(QResizeEvent *event);
    void showEvent(QShowEvent *);
    void hideEvent(QHideEvent *);
    void keyPressEvent(QKeyEvent *event);
    void keyReleaseEvent(QKeyEvent *event);
    void inputMethodEvent(QInputMethodEvent *e);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *);
    void wheelEvent(QWheelEvent *event);

private slots:
    void toggleCursorShow();
};

#endif // HOLERTEXT_H
