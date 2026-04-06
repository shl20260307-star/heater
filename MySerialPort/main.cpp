#include <iostream>
#include<widget.h>
#include<QApplication>
#include<mainwindow.h>
using namespace std;

int main(int argc, char *argv[])
{
    //这行代码创建了一个QApplication对象，它是每个Qt应用程序的起点。
    //argc和argv是从main函数传递过来的参数，分别表示命令行参数的数量和参数值。
    QApplication app(argc, argv);
    widget w;//创建了widget对象，是一个窗口组件
    w.show();//窗口w显示在屏幕上
    //这行代码启动Qt的事件循环。app.exec()会一直等待，直到应用程序关闭。
    //当窗口关闭时，事件循环结束，exec()函数返回，程序终止。
    return app.exec();
    cout<<"hello";//这句写在app.exec()之间才会被执行

}
