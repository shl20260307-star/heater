#include "wellplatedialog.h"
#include <QGridLayout>
#include <QPushButton>
#include <QMessageBox>
#include <QVector>
#include <QSerialPort>
#include <QDebug>
//新增
#include <QtMath>
#include <limits>


WellPlateDialog::WellPlateDialog(QWidget *parent)
    : QDialog(parent),
      wellStatus(new QVector<bool>(96, false)),
      confirmButton(new QPushButton("确认", this)),
      selectAllButton(new QPushButton("全选", this)),
      cancelAllButton(new QPushButton("取消全选", this)),
      serialPort(new QSerialPort(this))
{

    setWindowTitle("96 Well Plate");
    QGridLayout *layout = new QGridLayout(this);
    layout->setSpacing(5); // 控制孔之间的间距

    // 调整wellButtons大小，确保有96个按钮指针
    wellButtons.resize(96);

    isAllSelected=false; // 布尔类型的成员变量
    // 创建96个孔
    for (int row = 0; row < 8; ++row) {
        for (int col = 0; col < 12; ++col) {
             int index = row * 12 + col;//孔的索引
            QString wellLabel = QString("%1%2")
                .arg(QChar('A' + row))
                .arg(col + 1);
            QPushButton *wellButton = new QPushButton(wellLabel, this);
            wellButton->setFixedSize(40, 40); // 控制按钮大小
            wellButton->setToolTip("点击选择孔：" + wellLabel);
            wellButton->setCheckable(true); // 允许按钮可选中
            // 存储按钮指针
             wellButtons[index] = wellButton;

            // 将按钮添加到布局中
            layout->addWidget(wellButton, row, col);

            // 连接按钮的clicked信号到槽函数
            connect(wellButton, &QPushButton::clicked, [=]() {
                 if (wellButton->isChecked()) {
                      // 如果按钮被选中，改变按钮颜色并添加到选中列表
                       wellButton->setStyleSheet("background-color: lightblue;");
                       wellStatus->replace(index, true);
                  }else {

                      wellButton->setStyleSheet("");// 如果按钮被取消选中，恢复按钮颜色并从选中列表中移除
                      wellStatus->replace(index, false);
                  }


            });
           }
      }
    //全选按钮
    layout->addWidget(selectAllButton, 8, 0, 1, 6); // 按钮放在第9行，跨6列
    layout->addWidget(cancelAllButton, 8, 6, 1, 6); // 取消全选按钮放在第9行，跨6列
    // 添加确认按钮
     layout->addWidget(confirmButton, 9, 0, 1, 12); // 放在第10行，跨12列



     // 连接确认按钮的点击信号到槽函数
     connect(confirmButton, &QPushButton::clicked, this, &WellPlateDialog::onConfirmButtonClicked);
     connect(selectAllButton, &QPushButton::clicked, this, &WellPlateDialog::onselectAllButtonClicked);
     connect(cancelAllButton, &QPushButton::clicked, this, &WellPlateDialog::onCancelAllButtonClicked);
     setLayout(layout);
}
// 析构函数：释放内部分配的对象
WellPlateDialog::~WellPlateDialog() {
    delete wellStatus;
    delete serialPort;
}


// 从 wellStatus 中提取被选中的孔位，返回每个孔的 (行, 列)
// 计算方式：索引 = row * 12 + col，故 row = index / 12, col = index % 12
QVector<QPair<int,int>> WellPlateDialog::extractSelectedPositions() const
{
    QVector<QPair<int,int>> positions;
    for (int i = 0; i < 96; ++i) {
        if ((*wellStatus)[i]) {
            int row = i / 12;
            int col = i % 12;
            positions.append(qMakePair(row, col));
        }
    }
    return positions;
}
// 计算两个孔之间的欧几里得距离
static double pointDistance(const QPair<int,int>& a, const QPair<int,int>& b)
{
    return qSqrt(qPow(a.first - b.first, 2) + qPow(a.second - b.second, 2));
}
// 使用 Held–Karp 算法计算 TSP 最优路径（适用于选中孔位较少的情况），
//Held–Karp 算法复杂度为 O(2^N * N^2)，适用于选中孔位数量较少的情况（比如 10～15 个）；若选中数量很多，需要考虑其它近似算法。
// 参数 positions 中存储所有被选中孔位的 (行, 列)，返回在 positions 向量中的索引顺序
//计算最佳最短路径
QVector<int> WellPlateDialog::computeOptimalPath(const QVector<QPair<int,int>> &positions) const
{
    int N = positions.size();
    if (N == 0)
        return QVector<int>();

    const int FULL = 1 << N; // 状态数 2^N

    // dp[mask][i] 表示经过 mask 状态且最后到达 i 点的最短路径长度
    QVector<QVector<double>> dp(FULL, QVector<double>(N, std::numeric_limits<double>::max()));
    // parent[mask][i] 用于记录转移前的点，便于回溯路径
    QVector<QVector<int>> parent(FULL, QVector<int>(N, -1));

    // 设定起点为 positions[0]，初始状态 mask = 1<<0
    dp[1 << 0][0] = 0.0;

    // 遍历所有状态 mask
    for (int mask = 0; mask < FULL; ++mask) {
        for (int i = 0; i < N; ++i) {
            if (!(mask & (1 << i)))
                continue; // 当前状态不含点 i
            // 对于未访问的点 j
            for (int j = 0; j < N; ++j) {
                if (mask & (1 << j))
                    continue;  // 点 j 已经访问过，跳过
                int nextMask = mask | (1 << j);
                double newDist = dp[mask][i] + pointDistance(positions[i], positions[j]);
                if (newDist < dp[nextMask][j]) {
                    dp[nextMask][j] = newDist;
                    parent[nextMask][j] = i;
                }
            }
        }
    }

    // 找到状态 FULL-1（所有点已访问）中终点使得路径最短
    double minCost = std::numeric_limits<double>::max();
    int last = -1;
    for (int i = 0; i < N; ++i) {
        if (dp[FULL - 1][i] < minCost) {
            minCost = dp[FULL - 1][i];
            last = i;
        }
    }

    // 回溯求出最优路径（从终点向起点回溯）
    QVector<int> path;
    int mask = FULL - 1;
    while (last != -1) {
        path.prepend(last);
        int temp = parent[mask][last];
        mask = mask & ~(1 << last);  // 从 mask 中去掉 last
        last = temp;
    }
    // 返回的 path 中第一个点应为起点（对应 positions[0]）
    qDebug() << "Optimal path:" << path;
    return path;
}
// CRC16 校验函数
quint16 WellPlateDialog::calculateCRC16(const QByteArray &data) {

//    QString formattedData;
//    for (char byte : data) {
//        formattedData.append(QString("%1 ").arg(static_cast<unsigned char>(byte), 2, 16, QChar('0')).toUpper());
//    }
//    qDebug() << "Plate传入crc校验的数据:" << formattedData.trimmed();


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

    //qDebug() << "caiYANG-calculateCRC16: 0x" << QString::number(crc, 16).toUpper();

    return crc;
}



//}
// 采样数据帧格式：
// [帧头(0x3A)][帧长][功能码][数据区][CRC(2字节)][帧尾(0x0D, 0x0A)]
// 数据区格式： [孔数量(1字节)][孔1行(1字节)][孔1列(1字节)]...[孔N行][孔N列]
QByteArray WellPlateDialog::buildOptimizedCommandFrame(const QVector<QPair<int,int>> &positions,
                                                       const QVector<int> &pathOrder,
                                                       quint8 commandType) const
{
    QByteArray data;
    data.append(static_cast<quint8>(pathOrder.size()));// 数据区：首先添加孔数量（选中孔数量）
    for (int idx : pathOrder) {// 按照优化后的路径顺序，依次添加每个孔的行、列
        data.append(static_cast<char>(positions[idx].first));  // 行号（0~7）
        data.append(static_cast<char>(positions[idx].second)); // 列号（0~11）
    }
    // 构造完整帧
    QByteArray frame;
    frame.append(0x3A);// 帧头
    // 帧长
    quint16 lengthField = data.size() + 7;
    qDebug() << "帧长"<<lengthField;
    frame.append(lengthField);      // 帧长
    frame.append(commandType);      // 功能码（命令类型）
    frame.append(data);             // 数据区
    quint16 crc = calculateCRC16(frame);
    frame.append(static_cast<char>((crc >> 8) & 0xFF));  // CRC 高字节
    frame.append(static_cast<char>(crc & 0xFF));         // CRC 低字节
    frame.append(0x0D);  // 帧尾
    frame.append(0x0A);  // 帧尾

    //qDebug()<<"sendCommandPlate:"<<frame.toHex();
    return frame;
}


// 采样数据帧格式：
// [帧头(0x3A)][帧长][功能码][数据区][CRC(2字节)][帧尾(0x0D, 0x0A)]
// 数据区格式： [孔数量(1字节)][12 bytes：共96位表示96个孔，某位为1表示需要扫描该孔，为0则不扫描]
QByteArray WellPlateDialog::buildNonOptimizedCommandFrame(const QVector<bool>& bitmask){ //构建非优化路径命令帧
    QByteArray frame;
    frame.append(char(0x3A));               // 帧头
    const quint8 DATA_LEN = 13;  // 数据段固定 13=12+1 字节（每 8 个孔用一个字节）
    quint16 len = DATA_LEN +7;// 帧长
    frame.append(char(len));                 // 帧长
    frame.append(static_cast<quint8>(0x31));      // 功能码（命令类型）
    int count = std::count(bitmask.begin(), bitmask.end(), true);
    frame.append(char(count & 0xFF));

    // 拼 12 字节位图
    for (int byteIdx = 0; byteIdx < 12; ++byteIdx) {
        quint8 b = 0;
        for (int bit = 0; bit < 8; ++bit) {
            int idx = byteIdx*8 + bit;
            if (idx < 96 && bitmask[idx]) {
                b |= (1 << bit);
            }
        }
        frame.append(char(b));
    }

    // 计算 CRC 并补上
    quint16 crc = calculateCRC16(frame);
    frame.append(char((crc >> 8) & 0xFF));
    frame.append(char(crc & 0xFF));

    // 帧尾
    frame.append(char(0x0D));
    frame.append(char(0x0A));

    return frame;
}
// ---------- 原有槽函数修改 ----------
void WellPlateDialog::onConfirmButtonClicked() {//发送帧命令内容到单片机

    QByteArray command;
    // 1. 从 wellStatus 中提取选中孔位（以 (row, col) 形式保存）
    QVector<QPair<int,int>> selectedPositions = extractSelectedPositions();
    if (selectedPositions.isEmpty()) {
        QMessageBox::warning(this, "错误", "请至少选择一个孔位！");
        return;
    }
    int count = selectedPositions.size();
    if(count < 15){//孔位小于在0~14个之间，采取优化路径策略
             QVector<int> pathOrder;
            // 2. 使用 Held–Karp 算法计算最优路径
            pathOrder = computeOptimalPath(selectedPositions);
            // 3. 构造优化后的数据帧
            command = buildOptimizedCommandFrame(selectedPositions, pathOrder,static_cast<quint8>(0x31));


            // 输出调试信息：显示选中孔位及路径顺序
            QString selectedInfo = "选中的孔位：\n";
            for (int i = 0; i < selectedPositions.size(); ++i) {
                selectedInfo += QString("%1%2 ").arg(QChar('A' + selectedPositions[i].first))
                                  .arg(selectedPositions[i].second + 1);
            }
            qDebug() << selectedInfo;

            QString pathInfo = "优化路径（按按钮序号）：\n";
            for (int idx : pathOrder) {
                pathInfo += QString("%1 ").arg(idx);
            }
            qDebug() << pathInfo;

            // 输出数据帧信息（以十六进制显示）
            QString commandInfo = "发送的优化帧内容:\n";
            for (int i = 0; i < command.size(); ++i) {
                commandInfo += QString("%1").arg(static_cast<unsigned char>(command[i]), 2, 16, QChar('0'));
            }
            //qDebug() << commandInfo;


    }else{//孔数量多余14个，不进行优化路径
          // 2. 拿到位图
            auto& bits = *wellStatus;

            command = buildNonOptimizedCommandFrame(bits);


    }

    // 4. 发送数据帧：既通过信号发送给 widget，同时通过串口发送给单片机
    emit sendDataToWidget(command);

    QMessageBox::information(this, "发送成功", "已发送选中孔位的优化路径到单片机");


}

//// ---------- 原有槽函数修改 ----------
//void WellPlateDialog::onConfirmButtonClicked() {//发送帧命令内容到单片机


//    if (!isAllSelected) {
//        QVector<int> pathOrder;
//        QByteArray command;
//        // 1. 从 wellStatus 中提取选中孔位（以 (row, col) 形式保存）
//           QVector<QPair<int,int>> selectedPositions = extractSelectedPositions();
//           if (selectedPositions.isEmpty()) {
//               QMessageBox::warning(this, "错误", "请至少选择一个孔位！");
//               return;
//           }

//           // 2. 使用 Held–Karp 算法计算最优路径
//           // 注意：该算法适用于选中孔位较少的情况，如果选中孔位过多可能计算较慢
//           pathOrder = computeOptimalPath(selectedPositions);
//           // 3. 构造优化后的数据帧
//           command = buildOptimizedCommandFrame(selectedPositions, pathOrder,static_cast<quint8>(0x31));
//           // 输出调试信息：显示选中孔位及路径顺序
//           QString selectedInfo = "选中的孔位：\n";
//           for (int i = 0; i < selectedPositions.size(); ++i) {
//               selectedInfo += QString("%1%2 ").arg(QChar('A' + selectedPositions[i].first))
//                                 .arg(selectedPositions[i].second + 1);
//           }
//           qDebug() << selectedInfo;

//           QString pathInfo = "优化路径（按按钮序号）：\n";
//           for (int idx : pathOrder) {
//               pathInfo += QString("%1 ").arg(idx);
//           }
//           qDebug() << pathInfo;

//           // 输出数据帧信息（以十六进制显示）
//           QString commandInfo = "发送的优化帧内容:\n";
//           for (int i = 0; i < command.size(); ++i) {
//               commandInfo += QString("%1").arg(static_cast<unsigned char>(command[i]), 2, 16, QChar('0'));
//           }
//           //qDebug() << commandInfo;

//           // 4. 发送数据帧：既通过信号发送给 widget，同时通过串口发送给单片机
//           emit sendDataToWidget(command);

//           QMessageBox::information(this, "发送成功", "已发送选中孔位的优化路径到单片机");



//    }else{

//        QByteArray command;
//        // —— 全选时调用 12 字节的非优化帧构造 ——
//        command = buildNonOptimizedCommandFrame();
//        // 4. 发送数据帧：既通过信号发送给 widget，同时通过串口发送给单片机
//        emit sendDataToWidget(command);

//        QMessageBox::information(this, "发送成功", "已发送选中孔位的路径到单片机");

//    }



//}

void WellPlateDialog::onselectAllButtonClicked() {//全选按钮
    isAllSelected = true;  // 新增：设置全选标志为true
    // 选中所有孔
    for (int i = 0; i < 96; ++i) {
        (*wellStatus)[i] = true;
        wellButtons[i]->setChecked(true);
        wellButtons[i]->setStyleSheet("background-color: lightblue;");
    }

    // 显示消息框提示
    //QMessageBox::information(this, "全选", "所有孔已选中");
}
void WellPlateDialog::onCancelAllButtonClicked() {//取消全选
    isAllSelected = false;
    // 取消所有孔的选中状态
    for (int i = 0; i < 96; ++i) {
        (*wellStatus)[i] = false;
        wellButtons[i]->setChecked(false);
        wellButtons[i]->setStyleSheet("background-color: lightgray;"); // 取消选中后变成灰色
    }

    // 显示消息框提示
   // QMessageBox::information(this, "取消全选", "所有孔已取消选中");
}



