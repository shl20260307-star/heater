#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include<QtSerialPort/QSerialPort>
#include<QTimer>
#include"wellplatedialog.h"
#include "qcustomplot.h"  // QCustomPlot 的头文件
#include <QPushButton>
#include <QMap>



namespace Ui {
class widget;
}

class widget : public QWidget
{
    Q_OBJECT

public:
    explicit widget(QWidget *parent = 0);
    ~widget();


private slots://定义函数
    void TimerEvent();

    void on_pushButton_clicked();
    void serialPort_readyRead();

    void on_checkBox_clicked();

    void on_pushButton_2_clicked();

    void on_pushButton_3_clicked();

    void on_pushButton_4_clicked();
    void on_checkBox_2_clicked();
    void on_checkBox_3_clicked();

    //void readSerialData(); // 处理串口数据

    void on_pushButton_5_clicked();
    quint16 calculateCRC16(const QByteArray &data);
    void sendCommand(QSerialPort &serial,quint8 length,quint8 commandType, const QByteArray &data);
    void InitSerial();
    void sendTempRequest();

    //void on_pushButton_6_clicked();


    //void on_pushButton_8_clicked();

    //void on_pushButton_9_clicked();

   // void on_pushButton_10_clicked();

   // void on_pushButton_11_clicked();

    void on_pushButton_12_clicked();


//    void showWellPlate();

    void on_showPlateButton_clicked();

    void on_saveCsvButton_clicked();

private slots:
    void receiveDataFromDialog(const QByteArray &data);  // 用于接收 wellPlateDialog 发送的数据

    void on_checkBoxPlot_clicked();
    void on_checkBoxPrint_clicked();

 //  void onSaveToExcelClicked();  // 新增保存按钮槽函数



    void on_wheelPosition_clicked();


    // 三个按钮槽（新增）
    void on_togglePE0_clicked();
    void on_togglePI6_clicked();


    void on_setDacButton_clicked();


    void on_setTemperature_clicked();

    void on_stopHeatingCooling_clicked();//停止加热/制冷

    void on_selectLightSource_clicked();

    //void on_turnOnLight_clicked();

    //void on_turnOffLight_clicked();

    void on_checkTemperature_clicked();

    void on_signalAcquire_clicked();

private:
    Ui::widget *ui;
    QTimer *timer;//定义一个指针，用于周期性扫描接入的可选择的串口号
    QStringList portStringList;
    QSerialPort *serial;//定义一个串口对象
    QString receiveText;//串口接收到的数据
    QString sendText;//发送的数据
    long receive_Byte;//接收到的是几个字节
    long send_Byte;//发送的是几个字节

    QByteArray buffer;
    WellPlateDialog *wellPlateDialog; // 指向96位孔板窗口的指针

    QCustomPlot *customPlot;      // 用于绘图的 QCustomPlot 指针
    double lastTime;              // 记录上次更新时间，用于 X 轴滚动
    void setupPlot();             // 初始化绘图控件

    // 解耦后的处理函数
    void handlePrint(const QByteArray &line);
    void handlePlot(const QByteArray &line);

//    QVector<double> timeData, voltageData;   // 存储波形数据
    QVector<double> timeData;     // 存储时间数据
    QVector<double> voltageData;  // 存储电压数据
    QPushButton *saveButton;      // 保存按钮


    QTimer *tempTimer;      // 每秒发送测温度请求的定时器
    QLabel *tempLabel;      // 显示温度的控件
    bool tempSampling = false; // 是否正在采样

};

#endif // WIDGET_H
