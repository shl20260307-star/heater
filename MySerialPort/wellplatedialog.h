#ifndef WELLPLATEDIALOG_H
#define WELLPLATEDIALOG_H

#include <QDialog>
#include <QVector>
#include <QPushButton>
#include <QSerialPort>

class WellPlateDialog : public QDialog {
    Q_OBJECT

public:
    explicit WellPlateDialog(QWidget *parent = nullptr);
    ~WellPlateDialog();
private slots:
    void onConfirmButtonClicked();
    void onselectAllButtonClicked(); // 全选按钮的槽函数
    void onCancelAllButtonClicked(); // 取消全选按钮的槽函数

    static quint16 calculateCRC16(const QByteArray &data);


    QByteArray buildNonOptimizedCommandFrame(const QVector<bool>& bitmask); // 新增：构建非优化路径命令帧

    //void onToggleGpioClicked();



signals:
    void sendDataToWidget(const QByteArray &data);  // 定义信号，将数据发送到 widget

private:
    QVector<bool> *wellStatus; // 用于记录每个孔的状态（是否被选中）
    QVector<QPushButton*> wellButtons; // 存储96个按钮指针
    QPushButton *confirmButton; // 确认按钮
    QPushButton *selectAllButton; // 全选按钮
    QPushButton *cancelAllButton; // 取消全选按钮
    QSerialPort *serialPort;    // 串口通信对象
    bool isAllSelected; // 布尔类型的成员变量

    QPushButton *toggleGpioButton;    // 新增：翻转GPIO按钮


    // 新增私有函数
    // 从 wellStatus 中提取选中孔位，返回一个存储 (行, 列) 坐标的向量
    QVector<QPair<int, int>> extractSelectedPositions() const;
    // 使用 Held–Karp 算法计算 TSP 最优路径（适用于选中孔位较少的情况），返回在 positions 中的索引顺序
    QVector<int> computeOptimalPath(const QVector<QPair<int,int>> &positions) const;
    // 构造优化后的数据帧（数据区格式为：[孔数量][孔1行][孔1列]...），并按照 sendCommand 的格式包装：
    // 帧格式：[帧头(0x3A)][帧长][功能码][数据区][CRC(2字节)][帧尾(0x0D,0x0A)]
    QByteArray buildOptimizedCommandFrame(const QVector<QPair<int,int>> &positions,
                                            const QVector<int> &pathOrder,
                                            quint8 commandType) const;
};

#endif // WELLPLATEDIALOG_H
