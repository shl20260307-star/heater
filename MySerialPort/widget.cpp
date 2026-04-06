#include "widget.h"
#include "ui_widget.h"
#include <QDebug>
#include <QSerialPortInfo>
#include <QMessageBox>
#include "wellplatedialog.h"
#include <QPushButton>
#include <QVBoxLayout>
#include <QFileDialog>
#include <QStandardPaths>
#include <QIntValidator>
#include <QHBoxLayout>


widget::widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::widget),wellPlateDialog(nullptr)
{
    ui->setupUi(this);
    resize(1100,700);//指定窗口多大
    setWindowTitle("串口调试助手");
    timer =new QTimer(this);//为qtimer分配空间
    timer->start(500);//每500ms扫描可用串口号
    connect(timer,&QTimer::timeout,this,&widget::TimerEvent);//信号与槽之间连接，timer的时间timeout到了，执行timerEvent事件

    serial = new QSerialPort(this);
    InitSerial();
    connect(serial,QSerialPort::readyRead,this,&widget::serialPort_readyRead);//信号和槽
    ui->checkBox->setCheckState(Qt::Checked);//设置默认接受方式
    receive_Byte=0;//初始收到字节数为0
    send_Byte=0;//初始发送字节数为0

    // 获取 QCustomPlot 实例
     customPlot = ui->plotWidget;
     setupPlot();         // 初始化图表

     ui->labelTemperature->setText("--.- °C");
     ui->labelTemperature->setFixedSize(110,34);
     ui->labelTemperature->setAlignment(Qt::AlignCenter);
     ui->labelTemperature->setStyleSheet("background: white; color: red; border: 2px solid #000000; font-weight: bold; font-size: 16px;");
     tempTimer = new QTimer(this);//每1s请求一次单片机测实时温度
     tempTimer->setInterval(1000); // 1 秒
     connect(tempTimer, &QTimer::timeout, this, &widget::sendTempRequest);

     connect(serial, &QSerialPort::readyRead,this, &widget::serialPort_readyRead);// 串口有数据则处理

}

widget::~widget()
{
    delete wellPlateDialog;
       delete serial;
       delete timer;
    delete ui;
}
//函数实现
void widget::on_showPlateButton_clicked()
{
    if (!wellPlateDialog) {
        // 确保只创建一次
        wellPlateDialog = new WellPlateDialog(this);
        connect(wellPlateDialog, &WellPlateDialog::sendDataToWidget, this, &widget::receiveDataFromDialog);
    }
    // 显示 96 孔板界面
    wellPlateDialog->exec();


}
void widget::receiveDataFromDialog(const QByteArray &data)//采样信息在此处发送
{
    QString formattedData;
    for (char byte : data) {
        formattedData.append(QString("%1 ").arg(static_cast<unsigned char>(byte), 2, 16, QChar('0')).toUpper());
    }
    qDebug() << "接收到dialog的数据：" << formattedData.trimmed();

    serial->write(data);
    //检验数据是否成功
    //qint64 bytesWritten = serial->write(data);
    qDebug() << "尝试发送字节数：" << data.size() << "实际发送：" ;//<< bytesWritten;

    //qDebug() << "dialog：" << formattedData.trimmed();

    send_Byte += data.toHex().length();//发送字节处理
    ui->label_11->setText(QString::number(send_Byte));//Qnumber将整形转化成字符串类型
//    QByteArray hexData;
//    hexData.append(data);//命令内容
//    sendCommand(*serial,static_cast<quint8>(0x12), static_cast<quint8>(0x31),hexData);//字节长度，功能码
}

void widget::setupPlot()
{
    customPlot->addGraph();                    // 添加一条曲线
    customPlot->graph(0)->setPen(QPen(Qt::blue)); // 曲线颜色
    customPlot->xAxis->setLabel("Time (s)");    // X 轴标签
    customPlot->yAxis->setLabel("Voltage (V)"); // Y 轴标签
    customPlot->yAxis->setRange(0, 3.3);        // 电压范围 0～3.3V
    // 开启交互，可拖拽、缩放
    customPlot->setInteraction(QCP::iRangeDrag, true);
    customPlot->setInteraction(QCP::iRangeZoom, true);
}

void widget::handlePrint(const QByteArray &line)
{
    // 原有的累积显示逻辑
    ui->textEdit->append(QString::fromLatin1(line));//用于将一个 Latin-1编码的字节数组或 C 字符串转换成 QString（Unicode 字符串）

}

void widget::handlePlot(const QByteArray &line)
{
    bool ok = false;
    double voltage = line.toDouble(&ok);
    if (!ok) return;//ok 用来检测电压值转换是否成功，如果转换失败，则直接返回，不做后续绘图，保证数据安全。

    // 以程序启动时刻为基准计算 X 轴时间
    static double startTime = QDateTime::currentMSecsSinceEpoch() / 1000.0;
    double now = QDateTime::currentMSecsSinceEpoch() / 1000.0;
    double t = now - startTime;

    // 添加数据并重绘
    auto *plot = ui->plotWidget;        // 前提：已在 UI 中 promote 为 QCustomPlot*
    plot->graph(0)->addData(t, voltage);
    plot->xAxis->setRange(t, 5, Qt::AlignRight);
    plot->replot();


    // 存储数据
   timeData.append(t);
   voltageData.append(voltage);

   // 限制存储量（可选）
   if (timeData.size() > 3000) {  // 保留最近3千点
       timeData.removeFirst();
       voltageData.removeFirst();
   }
}

void widget::TimerEvent(){//定时扫描可用串口
    //qDebug() << "Hello";//检验定时器有没有生效
    QStringList newPortStringList;//可用串口列表
    newPortStringList.clear();
    foreach(const QSerialPortInfo &info,QSerialPortInfo::availablePorts())
        newPortStringList +=info.portName();

    if(newPortStringList.size() !=portStringList.size()){//处理下拉列表数据，解决其每500ms就重复性增加item问题
        portStringList = newPortStringList;
        ui->comboBox->clear();
        ui->comboBox->addItems(newPortStringList);
    }
}
void widget::InitSerial(){
    ui->comboBox_2->setCurrentIndex(4);//设置串口参数初始默认值
    ui->comboBox_3->setCurrentIndex(3);
    ui->comboBox_4->setCurrentIndex(2);
    ui->comboBox_5->setCurrentIndex(0);
    ui->comboBox_6->setCurrentIndex(0);
    ui->comboBox_7->setCurrentIndex(0);
}

void widget::on_pushButton_clicked()//槽函数，打开串口
{
    if(ui->pushButton->text() == QString("打开串口")){//设置波特率等信息
        serial->setPortName(ui->comboBox->currentText());//将串口号作为串口名字
        serial->setBaudRate(ui->comboBox_2->currentText().toInt());//波特率
        switch(ui->comboBox_3->currentText().toInt()){//数据位
            case 5:serial->setDataBits(QSerialPort::Data5);break;
            case 6:serial->setDataBits(QSerialPort::Data6);break;
            case 7:serial->setDataBits(QSerialPort::Data7);break;
            case 8:serial->setDataBits(QSerialPort::Data8);break;
            default:serial->setDataBits(QSerialPort::UnknownDataBits);
        }
        switch (ui->comboBox_4->currentIndex()) {//校验
        case 0:serial->setParity(QSerialPort::EvenParity);break;
        case 1:serial->setParity(QSerialPort::OddParity);break;
        case 2:serial->setParity(QSerialPort::NoParity);break;
        default:serial->setParity(QSerialPort::UnknownParity);
        }
        switch(ui->comboBox_5->currentIndex()){//停止位
            case 0:serial->setStopBits(QSerialPort::OneStop);break;
            case 1:serial->setStopBits(QSerialPort::TwoStop);break;
            default:serial->setStopBits(QSerialPort::UnknownStopBits);
        }
        serial->setFlowControl(QSerialPort::NoFlowControl);//设置流控
        if(!serial->open(QIODevice::ReadWrite)){//打开串口进行读写
            QMessageBox::information(this,"错误提示","无法打开串口",QMessageBox::Ok);//打开失败，弹出标准对话框
            return;
        }
        ui->comboBox->setEnabled(false);//串口打开之后，就不可以设置串口参数了
        ui->comboBox_2->setEnabled(false);
        ui->comboBox_3->setEnabled(false);
        ui->comboBox_4->setEnabled(false);
        ui->comboBox_5->setEnabled(false);

        ui->pushButton->setText("关闭串口");

    }else{
        serial->close();
        ui->comboBox->setEnabled(true);//串口关闭之后之后，可以设置串口参数了
        ui->comboBox_2->setEnabled(true);
        ui->comboBox_3->setEnabled(true);
        ui->comboBox_4->setEnabled(true);
        ui->comboBox_5->setEnabled(true);

        ui->pushButton->setText("打开串口");
    }

}
//当单片机或其他设备通过串口发送数据到电脑时，这些数据会被串口调试助手捕获，并显示在接收区
//数据传输的方向是：
//从发送区到单片机：用户在串口调试助手的发送区输入数据，并通过电脑的串口发送给单片机。
//从单片机到接收区：单片机或其他设备发送数据到电脑的串口，这些数据被串口调试助手接收，并显示在接收区
void widget::serialPort_readyRead(){//接收区：接收串口发送的数据

    QString last_text;
    QByteArray bytearrayReceive;//字节数组就是指字符串
    int length;//用于不断接收Hex时，本次与上次的数据分隔开
    if(ui->checkBox_3->checkState()!=Qt::Checked){

       // last_text = ui->textEdit->toPlainText();//读取上次串口接收到的数据

         // 这里改用按行读取，保证一次拿到一个完整数值文本
         while (serial->canReadLine()) {
               QByteArray line = serial->readLine().trimmed();//只发送shuju

                // 2. 按 UTF-8 解码成 QString
               QString str = QString::fromUtf8(line);

               // 3. 用 qDebug() 打印，中文可见
               qDebug() << "Received:" << str;

               //对采集到的温度数据进行处理显示
               if (str.startsWith("TEMPAVG:")) {
                   QString payload = str.mid(QString("TEMPAVG:").length()).trimmed();
                   bool ok = false;
                   double val = payload.toDouble(&ok);
                   if (ok) {
                       ui->labelTemperature->setText(QString::number(val, 'f', 4) + " °C");
                   } else {
                       ui->labelTemperature->setText("ERR00");
                   }
               } else if (str.startsWith("TEMP:")) {// 如果是温度响应行，格式为 "TEMP:xx.xx" 或 "TEMP:ERR_*"
                   QString payload = str.mid(5); // 去掉 "TEMP:"
                   if (payload.startsWith("ERR")) {
                       ui->labelTemperature->setText("ERR01");
                   } else {
                       bool ok = false;
                       double val = payload.toDouble(&ok);
                       if (ok) {
                           ui->labelTemperature->setText(QString::number(val, 'f', 4) + " °C");
                       } else {
                           ui->labelTemperature->setText("--.- °C");
                       }
                   }
               }

               //-----------------新增11.20 晚19:21
               if (str.startsWith("PE0:")) {
                   // 如果接收到的是 PE0 开头的数据
                   QRegExp rxPE0("PE0:(\\d)");  // 正则表达式提取 PE0 的电平值
                   if (rxPE0.indexIn(str) != -1) {
                       int pe0_state = rxPE0.cap(1).toInt();  // 提取 PE0 的电平状态

                       // 根据 PE0 的电平更新按钮
                       if (pe0_state == 1) {
                           ui->togglePE0->setText("关闭光电传感器");
                       } else {
                           //QMessageBox::warning(this, "错误", "未成功打开光电传感器");
                           ui->togglePE0->setText("打开光电传感器");
                       }
                   }
               } else if (str.startsWith("PI6:")) {
                   // 如果接收到的是 PI6 开头的数据
                   QRegExp rxPI6("PI6:(\\d)");  // 正则表达式提取 PI6 的电平值
                   if (rxPI6.indexIn(str) != -1) {
                       int pi6_state = rxPI6.cap(1).toInt();  // 提取 PI6 的电平状态

                       // 根据 PI6 的电平更新按钮
                       if (pi6_state == 1) {
                           ui->togglePI6->setText("关闭光源");
                       } else {
                           //QMessageBox::warning(this, "错误", "未成功打开光源");
                           ui->togglePI6->setText("打开光源");
                       }
                   }
               }


                // 根据复选框状态分发到不同功能
                if (ui->checkBoxPrint->isChecked()) {
                    handlePrint(line);//输出文本内容
                    receive_Byte +=line.length() + 1;
                    ui->label_10->setText(QString::number(receive_Byte));
                }
                if (ui->checkBoxPlot->isChecked()) {
                    handlePlot(line);//绘制波形图
                }

            }

            //receive_Byte +=line.length() + 1;
    }
}
// 新增保存函数实现
void widget::on_saveCsvButton_clicked()
{
    // 获取保存路径
    QString fileName = QFileDialog::getSaveFileName(
        this,
        "保存数据",
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
        "CSV Files (*.csv);;All Files (*)"
    );

    if (fileName.isEmpty()) return;

    // 打开文件
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "错误", "无法创建文件");
        return;
    }

    // 写入数据
    QTextStream out(&file);
    out << "时间(s),电压(V)\n";  // 表头
    for (int i = 0; i < timeData.size(); ++i) {
        out << QString::number(timeData[i], 'f', 4) << ","
            << QString::number(voltageData[i], 'f', 4) << "\n";
    }
    file.close();

    QMessageBox::information(this, "成功", "数据已保存至：" + fileName);
}


//控制只可以单选
void widget::on_checkBox_clicked()
{
    ui->checkBox->setCheckState(Qt::Checked);
    ui->checkBox_2->setCheckState(Qt::Unchecked);
    ui->checkBox_3->setCheckState(Qt::Unchecked);
}
void widget::on_checkBox_2_clicked()
{
    ui->checkBox->setCheckState(Qt::Unchecked);
    ui->checkBox_2->setCheckState(Qt::Checked);
    ui->checkBox_3->setCheckState(Qt::Unchecked);
}
void widget::on_checkBox_3_clicked()
{
    ui->checkBox->setCheckState(Qt::Unchecked);
    ui->checkBox_2->setCheckState(Qt::Unchecked);
    ui->checkBox_3->setCheckState(Qt::Checked);
}
void widget::on_checkBoxPlot_clicked()
{
    ui->checkBoxPlot->setCheckState(Qt::Checked);
    ui->checkBoxPrint->setCheckState(Qt::Unchecked);
}
void widget::on_checkBoxPrint_clicked()
{
    ui->checkBoxPlot->setCheckState(Qt::Unchecked);
    ui->checkBoxPrint->setCheckState(Qt::Checked);
}

//发送数据
void widget::on_pushButton_2_clicked()
{
    QByteArray bytearray;//字节数组就是指字符串
    sendText = ui->textEdit_2->toPlainText();//将接收到的数据转换为纯文本QString类型
    qDebug()<<"send_Text:"<<sendText;
    bytearray =QByteArray::fromHex(sendText.toUtf8());
   // bytearray = sendText.toLatin1();//将sendText字符串从Unicode转换为Latin1编码的字节序列，并存储在bytearray变量中,串口通信通常处理的是字节序列
    qDebug()<<"sendBytearray:"<<bytearray.toHex().toUpper();

    serial->write(bytearray);//发送数据到串口

    send_Byte += sendText.length();
    ui->label_11->setText(QString::number(send_Byte));//Qnumber将整形转化成字符串类型
}

void widget::on_pushButton_3_clicked()
{
    ui->textEdit->clear();
    receive_Byte=0;
    ui->label_10->setText(QString::number(receive_Byte));
}

void widget::on_pushButton_4_clicked()
{
    ui->textEdit_2->clear();
    send_Byte=0;
    ui->label_11->setText(QString::number(send_Byte));//Qnumber将整形转化成字符串类型
}

// CRC16 校验函数
quint16 widget::calculateCRC16(const QByteArray &data) {
//    QString formattedData;
//    for (char byte : data) {
//        formattedData.append(QString("%1 ").arg(static_cast<unsigned char>(byte), 2, 16, QChar('0')).toUpper());
//    }
    //qDebug() << "Widget传入CRC校验的数据:" << formattedData.trimmed();


    quint16 crc = 0xFFFF;
    for (char byte : data) {
        crc ^= static_cast<quint8>(byte);
        for (int i = 0; i < 8; i++) {
            if (crc & 0x0001)
                crc = (crc >> 1) ^ 0xA001;
            else
                crc >>= 1;
            //qDebug() << "Byte[" << i << "]: 0x" << QByteArray::number(data[i], 16) << ", CRC:" << crc;

        }
    }
    //qDebug()<<"Widget校验值calculateCRC16:"<<QString::number(crc, 16).toUpper();;

    return crc;
}

void widget::sendCommand(QSerialPort &serial,quint8 length,quint8 commandType, const QByteArray &data) {



    QByteArray frame;
    frame.append(0x3A); // 帧头
    frame.append(length+1);//帧长
    frame.append(commandType); // 命令类型
    frame.append(data); // 数据区
    // 计算 CRC
    quint16 crc = calculateCRC16(frame);
    frame.append(static_cast<char>((crc >> 8) & 0xFF)); // CRC 高字节
    frame.append(static_cast<char>(crc & 0xFF)); // CRC 低字节
    frame.append(0x0D); // 帧尾
    frame.append(0x0A); // 帧尾

    // 通过串口发送数据
    qDebug()<<"sendCommand:"<<frame.toHex();
    serial.write(frame);

    send_Byte += frame.toHex().length();//发送字节处理
    ui->label_11->setText(QString::number(send_Byte));//Qnumber将整形转化成字符串类型

}
void widget::on_pushButton_5_clicked()//开机
{

    QByteArray hexData;
    hexData.append(static_cast<char>(0x55));//命令内容
    sendCommand(*serial,static_cast<quint8>(0x07), static_cast<quint8>(0xF0),hexData);//字节长度，功能码
}

// ADDED: 每1秒发送测温度请求到 STM32（功能码 0xB7） ===
void widget::sendTempRequest()
{
    QByteArray hexData;
    hexData.append(static_cast<char>(0x55));
    sendCommand(*serial, static_cast<quint8>(0x07), static_cast<quint8>(0xB7), hexData);
    // sendCommand 已经会更新 send_Byte 和界面显示
}

// === MODIFIED ===
void widget::on_checkTemperature_clicked()
{
    if (!tempSampling) {
        // 开始采样：立即发送一次请求并启动定时器
        sendTempRequest();
        tempTimer->start();
        tempSampling = true;
        ui->checkTemperature->setText("停止测温");
        // 初始化显示
        ui->labelTemperature->setText("--.- °C");
    } else {
        // 停止采样
        tempTimer->stop();
        tempSampling = false;
        ui->checkTemperature->setText("测实时温度");
    }
}

void widget::on_stopHeatingCooling_clicked()//停止加热制冷
{
    QByteArray hexData;
    hexData.append(static_cast<char>(0x55));//命令内容
    sendCommand(*serial,static_cast<quint8>(0x07), static_cast<quint8>(0xB0),hexData);//字节长度，功能码
}

void widget::on_togglePI6_clicked()//翻转PI6（175脚）
{
    // mask = 0x02 表示 PI6
    QByteArray hexData;
    hexData.append(static_cast<char>(0x02));//命令内容
    sendCommand(*serial,static_cast<quint8>(0x07), static_cast<quint8>(0xA1),hexData);//字节长度，功能码
    qDebug()<<"PI6:";
}
//void widget::on_turnOnLight_clicked()//光源开电
//{
//    QByteArray hexData;
//    hexData.append(static_cast<char>(0x55));//命令内容
//    sendCommand(*serial,static_cast<quint8>(0x07), static_cast<quint8>(0x03),hexData);//字节长度，功能码
//}

//void widget::on_turnOffLight_clicked()//光源关电
//{
//    QByteArray hexData;
//    hexData.append(static_cast<char>(0x55));//命令内容
//    sendCommand(*serial,static_cast<quint8>(0x07), static_cast<quint8>(0x08),hexData);//字节长度，功能码
//}

//void widget::on_pushButton_10_clicked()//光强设置
//{
//    QByteArray hexData;
//    QString illuminance;//光强数据
//    illuminance = ui->textEdit_3->toPlainText();//将接收到的数据转换为纯文本QString类型
//    illuminance = illuminance.remove("0X");
//    qDebug()<<"illuminance:"<<illuminance;

//    // 确保文本只包含有效的十六进制字符
//    QRegExp hexRegExp("^[0-9A-Fa-f]+$");//QRegExp hexRegExp("^[0-9A-Fa-f]+$");
//    if (!hexRegExp.exactMatch(illuminance)) {
//        // 处理错误或发出警告
//    }

//    // 将十六进制字符串转换为QByteArray
//    QByteArray qb = QByteArray::fromHex(illuminance.toUtf8());
//    hexData.append(qb);//命令内容
//    sendCommand(*serial,static_cast<quint8>(0x07), static_cast<quint8>(0x0C),hexData);//字节长度，功能码
//}

void widget::on_selectLightSource_clicked()//光源波段选择
{
    QByteArray hexData;
    // 获取选中的文本
    int selectedIndex = ui->comboBox_6->currentIndex();
    if(selectedIndex == 0){
        hexData.append(static_cast<char>(0x44));
    }else{
        hexData.append(static_cast<char>(0x66));

    }
    sendCommand(*serial,static_cast<quint8>(0x07), static_cast<quint8>(0x06),hexData);//字节长度，功能码


}

void widget::on_pushButton_12_clicked()//弹出、加载、震荡
{
    QByteArray hexData;
    // 获取选中的文本
    int selectedIndex = ui->comboBox_7->currentIndex();
    if(selectedIndex == 0){
        hexData.append(static_cast<char>(0x00));
    }else if(selectedIndex == 1){
        hexData.append(static_cast<char>(0xFF));

    }else{
        hexData.append(static_cast<char>(0xF0));

    }
    sendCommand(*serial,static_cast<quint8>(0x07), static_cast<quint8>(0x37),hexData);//字节长度，功能码

}

void widget::on_setTemperature_clicked()
{
    QByteArray hexData;
    QString temperature,time;//温度数据
    temperature = ui->textEdit_4->toPlainText();//将接收到的数据转换为纯文本QString类型
    time = ui->textEdit_5->toPlainText();//将接收到的数据转换为纯文本QString类型

    temperature = temperature.remove("0X");
    time = time.remove("0X");

    // 确保文本只包含有效的十六进制字符
    QRegExp hexRegExp("^[0-9A-Fa-f]+$");//QRegExp hexRegExp("^[0-9A-Fa-f]+$");
    if (!hexRegExp.exactMatch(temperature) && !hexRegExp.exactMatch(time)) {
        // 处理错误或发出警告
    }

    // 将十六进制字符串转换为QByteArray
    QByteArray qbTemp = QByteArray::fromHex(temperature.toUtf8());
    QByteArray qbTime = QByteArray::fromHex(time.toUtf8());

    hexData.append(qbTemp);//命令内容
    hexData.append(qbTime);//命令内容

    sendCommand(*serial,static_cast<quint8>(0x08), static_cast<quint8>(0xB0),hexData);//字节长度，功能码

}

void widget::on_wheelPosition_clicked()
{
    QByteArray hexData;
    hexData.append(static_cast<char>(0x55));//命令内容
    sendCommand(*serial,static_cast<quint8>(0x07), static_cast<quint8>(0xC8),hexData);//字节长度，功能码
}



void widget::on_togglePE0_clicked()//翻转PE0（169脚）
{

    //mask = 0x01 表示 PE0
    QByteArray hexData;
    hexData.append(static_cast<char>(0x01));//命令内容
    sendCommand(*serial,static_cast<quint8>(0x07), static_cast<quint8>(0xA1),hexData);//字节长度，功能码
    qDebug()<<"PE0:";
}

void widget::on_setDacButton_clicked()
{
    QString s = ui->dacValueEdit->toPlainText();
    qDebug() << "DAC2值为:" << s;
    if (s.isEmpty()) {
        QMessageBox::warning(this, "输入错误", "请输入 DAC 值（0 ~ 4095）。");
        return;
    }

    bool ok = false;
    int val = 0;
    // 支持十六进制 0x 前缀和十进制
    if (s.startsWith("0x", Qt::CaseInsensitive)) {
        val = s.mid(2).toInt(&ok, 16);
    } else {
        val = s.toInt(&ok, 10);
        // 若字符串包含 A-F 等十六进制字符，但用户没有加 0x 前缀，尝试当 hex 解析
        if (!ok && QRegExp("^[0-9A-Fa-f]+$").exactMatch(s)) {
            val = s.toInt(&ok, 16);
        }
    }

    if (!ok) {
        QMessageBox::warning(this, "输入错误", "无法识别输入，请使用 0~4095 的十进制或以 0x 开头的十六进制。");
        return;
    }

    if (val < 0) val = 0;
    if (val > 4095) val = 4095;

    // 构造数据区：DH DL
    QByteArray data;
    quint16 uval = static_cast<quint16>(val);
    data.append(static_cast<char>((uval >> 8) & 0xFF));
    data.append(static_cast<char>(uval & 0xFF));


    sendCommand(*serial,static_cast<quint8>(0x08), static_cast<quint8>(0x0C),data);//字节长度，功能码


    // 更新发送字节计数（sendCommand 本身也会累加，但为了保险这里也更新显示）
    // send_Byte 已在 sendCommand 中被更新（append hex 长度），但保持同步：
    ui->label_11->setText(QString::number(send_Byte));

//    // 显示提示（不涉及上位机等待回应，因为你要求下位机不回传）
//    double vout = 3.3 * (double)uval / 4095.0; // 若你的 Vref 是 2.5V，请改为 2.5
//    ui->statusbar->showMessage(QString("已发送 DAC=%1 (≈%2 V)").arg(uval).arg(vout, 0, 'f', 3), 3000);
}

void widget::on_signalAcquire_clicked()
{
    if(ui->signalAcquire->text() == "开始信号采集")
     {
            ui->signalAcquire->setText("结束信号采集");

            QByteArray hexData;
            hexData.append(static_cast<char>(0x55));
            sendCommand(*serial,static_cast<quint8>(0x07), static_cast<quint8>(0xD4),hexData);//字节长度，功能码
    }
     else
     {
            ui->signalAcquire->setText("开始信号采集");


   }


}
