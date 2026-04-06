#include "mainwindow.h"
#include "wellplatedialog.h"
#include <QPushButton>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent)
    : QWidget(parent) {
    QPushButton *button = new QPushButton("显示96孔板", this);
    connect(button, &QPushButton::clicked, [=]() {
        WellPlateDialog *dialog = new WellPlateDialog(this);
        dialog->exec();
    });

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(button);
    setLayout(layout);
}
