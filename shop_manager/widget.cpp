#include "widget.h"
#include <pieview.h>
#include "ui_widget.h"
#include <QtSql>
#include <QMessageBox>
#include <QSplitter>
#include <QDateTime>
#include <QStandardItemModel>
#include <QTableView>
#include <QDebug>

Widget::Widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget)
{
    ui->setupUi(this);
    setFixedSize(750,500);
    ui->stackedWidget->setCurrentIndex(0);

    QSqlQueryModel *typeModel = new QSqlQueryModel(this);
    typeModel->setQuery("select name from type");
    ui->sellTypeComboBox->setModel(typeModel);

    //添加分裂器--可以拖动
    QSplitter *splitter = new QSplitter(ui->managePage);
    splitter->resize(700, 360);
    splitter->move(0, 50);


    splitter->addWidget(ui->toolBox);
    splitter->addWidget(ui->dailyList);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 1);

    on_sellCancelBtn_clicked();

    showDailyList();

    ui->typeComboBox_2->setModel(typeModel);
    createChartModelView();

}

Widget::~Widget()
{
    delete ui;
}

//商品类型发生变化
void Widget::on_sellTypeComboBox_currentIndexChanged(const QString &type)
{
    if (type == "请选择类型") {
        // 进行其他部件的状态设置
        on_sellCancelBtn_clicked();
    } else {
        ui->sellBrandComboBox->setEnabled(true);
        QSqlQueryModel *model = new QSqlQueryModel(this);
        model->setQuery(QString("select name from brand where type='%1'").arg(type));
        ui->sellBrandComboBox->setModel(model);
        ui->sellCancelBtn->setEnabled(true);
    }
}

//商品品牌发生变化
void Widget::on_sellBrandComboBox_currentIndexChanged(const QString &brand)
{

    ui->sellNumSpinBox->setValue(0);
    ui->sellNumSpinBox->setEnabled(false);
    ui->sellSumLineEdit->clear();
    ui->sellSumLineEdit->setEnabled(false);
    ui->sellOkBtn->setEnabled(false);

    QSqlQuery query;
    query.exec(QString("select price from brand where name='%1' and type='%2'")
               .arg(brand).arg(ui->sellTypeComboBox->currentText()));
    query.next();
    ui->sellPriceLineEdit->setEnabled(true);
    ui->sellPriceLineEdit->setReadOnly(true);
    ui->sellPriceLineEdit->setText(query.value(0).toString());

    query.exec(QString("select last from brand where name='%1' and type='%2'")
               .arg(brand).arg(ui->sellTypeComboBox->currentText()));
    query.next();
    int num = query.value(0).toInt();

    if (num == 0) {
        QMessageBox::information(this, tr("提示"), tr("该商品已经售完！"), QMessageBox::Ok);
    } else {
        ui->sellNumSpinBox->setEnabled(true);
        ui->sellNumSpinBox->setMaximum(num);
        ui->sellLastNumLabel->setText(tr("剩余数量：%1").arg(num));
        ui->sellLastNumLabel->setVisible(true);
    }
}

//商品数量发生变化
void Widget::on_sellNumSpinBox_valueChanged(int value)
{

    if (value == 0) {
        ui->sellSumLineEdit->clear();
        ui->sellSumLineEdit->setEnabled(false);
        ui->sellOkBtn->setEnabled(false);
    } else {
        ui->sellSumLineEdit->setEnabled(true);
        ui->sellSumLineEdit->setReadOnly(true);
        qreal sum = value * ui->sellPriceLineEdit->text().toInt();
        ui->sellSumLineEdit->setText(QString::number(sum));
        ui->sellOkBtn->setEnabled(true);
    }

}

void Widget::on_sellCancelBtn_clicked()
{

    ui->sellTypeComboBox->setCurrentIndex(0);
    ui->sellBrandComboBox->clear();
    ui->sellBrandComboBox->setEnabled(false);
    ui->sellPriceLineEdit->clear();
    ui->sellPriceLineEdit->setEnabled(false);
    ui->sellNumSpinBox->setValue(0);
    ui->sellNumSpinBox->setEnabled(false);
    ui->sellSumLineEdit->clear();
    ui->sellSumLineEdit->setEnabled(false);
    ui->sellOkBtn->setEnabled(false);
    ui->sellCancelBtn->setEnabled(false);
    ui->sellLastNumLabel->setVisible(false);

}

void Widget::on_sellOkBtn_clicked()
{
    QString type = ui->sellTypeComboBox->currentText();
    QString name = ui->sellBrandComboBox->currentText();
    int value = ui->sellNumSpinBox->value();
    // cellNumSpinBox的最大值就是以前的剩余量
    int last = ui->sellNumSpinBox->maximum() - value;

    QSqlQuery query;

    // 获取以前的销售量
    query.exec(QString("select sell from brand where name='%1' and type='%2'")
               .arg(name).arg(type));
    query.next();
    int sell = query.value(0).toInt() + value;

    // 事务操作
    QSqlDatabase::database().transaction();
    bool rtn = query.exec(
                QString("update brand set sell=%1,last=%2 where name='%3' and type='%4'")
                .arg(sell).arg(last).arg(name).arg(type));

    if (rtn) {
        QSqlDatabase::database().commit();
        QMessageBox::information(this, tr("提示"), tr("购买成功！"), QMessageBox::Ok);
        writeXml();
        showDailyList();
        on_sellCancelBtn_clicked();
    } else {
        QSqlDatabase::database().rollback();
    }

}


QString Widget::getDateTime(DateTimeType type)
{
    QDateTime datetime = QDateTime::currentDateTime();
    QString date = datetime.toString("yyyy-MM-dd");      //时间格式
    QString time = datetime.toString("hh:mm");          //小时间格式
    QString dateAndTime = datetime.toString("yyyy-MM-dd dddd hh:mm");    //合并格式
    if(type == Date)
        return date;
    else if(type == Time)
        return time;
    else
        return dateAndTime;
}

//读取xml文档
bool Widget::docRead()
{
    QFile file("data.xml");
    if(!file.open(QIODevice::ReadOnly))
        return false;
    if(!doc.setContent(&file))          //重复写入内容
    {
        file.close();
        return false;
    }
    file.close();
    return true;
}

//写入xml文档
bool Widget::docWrite()
{
    QFile file("data.xml");
    if(!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return false;
    QTextStream out(&file);
    doc.save(out,4);            //保存文件加每个缩进值
    file.close();
    return true;

}

//将销售记录写入文档
void Widget::writeXml()
{
    //从文件开始读取
    if(docRead())
    {
        QString currentDate = getDateTime(Date);
        QDomElement root = doc.documentElement();
        //判断日期节点进行处理
        if(!root.hasChildNodes())
        {
           QDomElement date = doc.createElement(QString("日期"));
           QDomAttr curDate = doc.createAttribute("date");
           curDate.setValue(currentDate);
           date.setAttributeNode(curDate);
           root.appendChild(date);
           createNodes(date);
        } else
        {
            QDomElement date = root.lastChild().toElement();
            //判断是否有今天的日期节点
            if(date.attribute("date") == currentDate)
            {
                createNodes(date);
            } else
            {
                QDomElement date = doc.createElement(QString("日期"));
                QDomAttr curDate = doc.createAttribute("date");
                curDate.setValue(currentDate);
                date.setAttributeNode(curDate);
                root.appendChild(date);
                createNodes(date);
            }
        }
        docWrite();

    }
}

//创建节点
void Widget::createNodes(QDomElement &date)
{
    QDomElement time = doc.createElement(QString("时间"));
    QDomAttr curTime = doc.createAttribute("time");
    curTime.setValue(getDateTime(Time));
    time.setAttributeNode(curTime);
    date.appendChild(time);
    QDomElement type = doc.createElement(QString("类型"));        //type
    QDomElement brand = doc.createElement(QString("品牌"));       //brand
    QDomElement price = doc.createElement(QString("单价"));       //price
    QDomElement num = doc.createElement(QString("数量"));         //num
    QDomElement sum = doc.createElement(QString("金额"));         //sum
    QDomText text;
    //type
    text = doc.createTextNode(QString("%1").arg(ui->sellTypeComboBox->currentText()));
    type.appendChild(text);
    //brand
    text = doc.createTextNode(QString("%1").arg(ui->sellBrandComboBox->currentText()));
    brand.appendChild(text);
    //price
    text = doc.createTextNode(QString("%1").arg(ui->sellPriceLineEdit->text()));
    price.appendChild(text);
    //num
    text = doc.createTextNode(QString("%1").arg(ui->sellNumSpinBox->value()));
    num.appendChild(text);
    //sum
    text = doc.createTextNode(QString("%1").arg(ui->sellSumLineEdit->text()));
    sum.appendChild(text);
    //添加进入时间
    time.appendChild(type);
    time.appendChild(brand);
    time.appendChild(price);
    time.appendChild(num);
    time.appendChild(sum);

}

//显示定义
void Widget::showDailyList()
{
    ui->dailyList->clear();         //首先清空
    if(docRead())
    {
        QDomElement root = doc.documentElement();
        QString title = root.tagName();     //返回标记名字
        QListWidgetItem *titleItem= new QListWidgetItem;
        titleItem->setText(QString("-----%1------").arg(title));        //设置标题
        titleItem->setTextAlignment(Qt::AlignCenter);                     //居中对齐
        ui->dailyList->addItem(titleItem);
        if(root.hasChildNodes())
        {
            QString currentDate = getDateTime(Date);
            QDomElement dateElement = root.lastChild().toElement();
            QString date = dateElement.attribute("date");
            if(date == currentDate)
            {
                ui->dailyList->addItem("");
                ui->dailyList->addItem(QString("日期:%1").arg(date));
                ui->dailyList->addItem("");
                QDomNodeList children = dateElement.childNodes();
                //遍历当日销售的所有商品
                for(int i = 0; i < children.count(); i++)
                {
                    QDomNode node = children.at(i);
                    QString time = node.toElement().attribute("time");
                    QDomNodeList list = node.childNodes();
                    QString type = list.at(0).toElement().text();
                    QString brand = list.at(1).toElement().text();
                    QString price = list.at(2).toElement().text();
                    QString num = list.at(3).toElement().text();
                    QString sum = list.at(4).toElement().text();

                    QString str = time + " 出售 " + brand + type
                            + " " + num + "台， " + "单价：" + price
                            + "元， 共" + sum + "元";
                    QListWidgetItem *tempItem = new QListWidgetItem;
                    tempItem->setText("**************************");
                    tempItem->setTextAlignment(Qt::AlignCenter);
                    ui->dailyList->addItem(tempItem);
                    ui->dailyList->addItem(str);
                }
            }
        }

    }
}

//创建销售记录的 模型和视图
void Widget::createChartModelView()
{
    chartModel = new QStandardItemModel(this);
    chartModel->setColumnCount(2);
    chartModel->setHeaderData(0, Qt::Horizontal, QString("品牌"));
    chartModel->setHeaderData(1, Qt::Horizontal, QString("销售数量"));

    QSplitter *splitter = new QSplitter(ui->charPage);
    splitter->resize(700, 320);
    splitter->move(0, 80);
    QTableView *table = new QTableView;
    PieView *pieChart = new PieView(ui->charPage);
    splitter->addWidget(table);
    splitter->addWidget(pieChart);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 2);

    table->setModel(chartModel);
    pieChart->setModel(chartModel);

    QItemSelectionModel *selectionModel = new QItemSelectionModel(chartModel);
    table->setSelectionModel(selectionModel);
    pieChart->setSelectionModel(selectionModel);
}

//销售记录显示图表
void Widget::showChart()
{
    qDebug() << "到了显示3!";
    QSqlQuery query;
    query.exec(QString("select name,sell from brand where type='%1'")
               .arg(ui->typeComboBox_2->currentText()));

    chartModel->removeRows(0, chartModel->rowCount(QModelIndex()), QModelIndex());

    int row = 0;

    while(query.next()) {
        int r = qrand() % 256;
        int g = qrand() % 256;
        int b = qrand() % 256;

        chartModel->insertRows(row, 1, QModelIndex());


        chartModel->setData(chartModel->index(row, 0, QModelIndex()),
                            query.value(0).toString());
        chartModel->setData(chartModel->index(row, 1, QModelIndex()),
                            query.value(1).toInt());
        chartModel->setData(chartModel->index(row, 0, QModelIndex()),
                            QColor(r, g, b), Qt::DecorationRole);
        row++;
    }
}

//销售统计类型选择框
void Widget::on_typeComboBox_2_currentIndexChanged(const QString &type)
{
    if (type != "请选择类型")
        showChart();
    qDebug() << "到了显示！";
}

//更新按钮
void Widget::on_updateBtn_2_clicked()
{
    if (ui->typeComboBox_2->currentText() != "请选择类型")
        showChart();
    qDebug() << "到了显示2！";

}

//商品管理按钮
void Widget::on_manageBtn_clicked()
{
    ui->stackedWidget->setCurrentIndex(0);
}

//销售统计按钮
void Widget::on_chartBtn_clicked()
{
    ui->stackedWidget->setCurrentIndex(1);
}
