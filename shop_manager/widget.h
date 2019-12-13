#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QDomDocument>

class QStandardItemModel;

namespace Ui {
class Widget;
}

class Widget : public QWidget
{
    Q_OBJECT

public:
    explicit Widget(QWidget *parent = nullptr);
    ~Widget();
    enum DateTimeType{Time, Date, DateTime};
    QString getDateTime(DateTimeType type);         //用以获取当前的时间和日期

private slots:
    void on_sellTypeComboBox_currentIndexChanged(const QString &arg1);

    void on_sellBrandComboBox_currentIndexChanged(const QString &arg1);

    void on_sellNumSpinBox_valueChanged(int arg1);

    void on_sellCancelBtn_clicked();

    void on_sellOkBtn_clicked();

    void on_typeComboBox_2_currentIndexChanged(const QString &arg1);

    void on_updateBtn_2_clicked();

    void on_manageBtn_clicked();

    void on_chartBtn_clicked();

private:
    Ui::Widget *ui;
    QDomDocument doc;

    QStandardItemModel *chartModel;

private:
    bool docRead();                         //文件读
    bool docWrite();                        //文件写
    void writeXml();                        //写入xml文件
    void createNodes(QDomElement &date);    //创建根节点
    void showDailyList();                   //显示

    void createChartModelView();
    void showChart();
};

#endif // WIDGET_H
