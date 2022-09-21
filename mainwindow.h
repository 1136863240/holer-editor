#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "holertext.h"
#include <QMainWindow>
#include <QResizeEvent>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

protected:
    void resizeEvent(QResizeEvent *event);

private:
    Ui::MainWindow *ui;
    HolerText *holer_text;
};

#endif // MAINWINDOW_H
