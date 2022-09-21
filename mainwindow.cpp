#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QVariant>
#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <QString>
#include <QStringList>
#include <QMessageBox>
#include <QFileInfo>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    QStringList args = QApplication::arguments();
    QString arg, content = "";
    int arg_length = args.length();
    if (arg_length > 2) {
        QMessageBox::critical(this, "系统错误", "只能传入一个参数");
        exit(-1);
    } else if (arg_length == 2) {
        arg = args.at(1);
        QFileInfo info(arg);
        if (info.isFile()) {
            QFile file(arg);
            if (!file.exists()) {
                QMessageBox::critical(this, "系统错误", "文件不存在");
                exit(-1);
            }
            if (!file.open(QFile::ReadOnly | QFile::Text)) {
                QMessageBox::critical(this, "系统错误", "文件打不开");
                exit(-1);
            }
            QTextStream ts(&file);
            content = ts.readAll();
            file.close();
        } else if (info.isDir()) {
            //
        }
    }

    this->holer_text = new HolerText(this);
    this->holer_text->setCanvasSize(0, 0, QSize(this->width(), this->height()));
    this->holer_text->setShowNumber(true);
    this->holer_text->setCanvasColor(QColor(0, 0, 0));
    this->holer_text->setNumberBackgroundColor(QColor(255, 255, 255));
    this->holer_text->setNumberColor(QColor(0, 0, 0));
    this->holer_text->setCursorColor(QColor(255, 255, 255));
    this->holer_text->setText(content);
    this->holer_text->setCursorPoint(0, 0);
    this->holer_text->setShowPoint(0, 0);
    this->holer_text->setDefaultColor(QColor(255, 255, 255));
    this->holer_text->setSelectedBackgroundColor(QColor(255, 255, 0));
    this->holer_text->setSelectedColor(QColor(0, 0, 0));
    this->holer_text->addPluginFile("./plugins/key/up.js", "key-up");
    this->holer_text->addPluginFile("./plugins/key/down.js", "key-down");
    this->holer_text->addPluginFile("./plugins/key/left.js", "key-left");
    this->holer_text->addPluginFile("./plugins/key/right.js", "key-right");
    this->holer_text->addPluginFile("./plugins/key/end.js", "key-end");
    this->holer_text->addPluginFile("./plugins/key/_b.js", "key-\\b");
    this->holer_text->addPluginFile("./plugins/key/_r.js", "key-\\r");
    this->holer_text->addPluginFile("./plugins/key/_u0016.js", "key-\\u0016");  // Ctrl + V
    this->holer_text->addPluginFile("./plugins/key/_u007F.js", "key-\\u007F");  // Delete
    this->holer_text->addPluginFile("./plugins/key/_u0003.js", "key-\\u0003");  // Ctrl + C
    this->holer_text->setKeyPluginInt(Qt::Key_Up, "key-up");
    this->holer_text->setKeyPluginInt(Qt::Key_Down, "key-down");
    this->holer_text->setKeyPluginInt(Qt::Key_Left, "key-left");
    this->holer_text->setKeyPluginInt(Qt::Key_Right, "key-right");
    this->holer_text->setKeyPluginInt(Qt::Key_End, "key-end");
    this->holer_text->setKeyPluginQString("\b", "key-\\b");
    this->holer_text->setKeyPluginQString("\r", "key-\\r");
    this->holer_text->setKeyPluginQString("\u0016", "key-\\u0016");
    this->holer_text->setKeyPluginQString("\u007F", "key-\\u007F");
    this->holer_text->setKeyPluginQString("\u0003", "key-\\u0003");
    this->holer_text->show();
}

MainWindow::~MainWindow()
{
    delete this->holer_text;
    delete ui;
}

void MainWindow::resizeEvent(QResizeEvent *event) {
    QSize new_size = event->size();
    this->holer_text->setCanvasSize(0, 0, QSize(new_size.width(), new_size.height()));
}
