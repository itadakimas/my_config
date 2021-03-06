#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    this->loadSettings();
    this->saveSettings();

    this->initTab();

    //Ram data & graph
    QJsonObject dataRam;

    QChart *chart = new QChart();
    chart->legend()->hide();
    chart->setTitle("RAM Disponible");

    chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);
    ui->verticalLayout_2->addWidget(chartView);

    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(refreshRam()));
    timer->start(this->ms_time);
}

void MainWindow::loadSettings()
{
    QSettings settings(QFileInfo(QCoreApplication::applicationFilePath()).absoluteFilePath() + QString::fromStdString("settings.ini"), QSettings::IniFormat);
    this->url_api = settings.value("url_api").toString();
    this->ms_time = settings.value("ms_time", 2000).toString().toInt();
}

void MainWindow::saveSettings()
{
    QSettings settings(QFileInfo(QCoreApplication::applicationFilePath()).absoluteFilePath() + QString::fromStdString("settings.ini"), QSettings::IniFormat);
    settings.setValue("url_api", this->url_api);
    settings.setValue("ms_time", this->ms_time);
}

void MainWindow::initTab()
{
    if (this->url_api.toStdString() != "") {

        Request* request_cpu = new Request();
        request_cpu->get(this->url_api + QString::fromStdString("/cpu"));
        connect(request_cpu->manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(loadCpu(QNetworkReply*)));

        Request* request_os = new Request();
        request_os->get(this->url_api + QString::fromStdString("/os"));
        connect(request_os->manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(loadOs(QNetworkReply*)));

        Request* request_ram = new Request();
        request_ram->get(this->url_api + QString::fromStdString("/ram"));
        connect(request_ram->manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(loadRam(QNetworkReply*)));

        Request* request_hdd = new Request();
        request_hdd->get(this->url_api + QString::fromStdString("/hdd"));
        connect(request_hdd->manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(loadHdd(QNetworkReply*)));
    }
}

void MainWindow::refreshRam()
{
    if (this->url_api != "") {
        Request* request_ram = new Request();
        request_ram->get(this->url_api + QString::fromStdString("/ram"));
        connect(request_ram->manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(loadRam(QNetworkReply*)));
    }
}

void MainWindow::loadCpu(QNetworkReply* reply)
{
    if(reply->error())
    {
        qDebug() << "ERROR!";
        qDebug() << reply->errorString();
    }
    else
    {
        QString strReply = (QString)reply->readAll();
        QJsonDocument jsonResponse = QJsonDocument::fromJson(strReply.toUtf8());
        QJsonObject jsonObj = jsonResponse.object();
        ui->cpu_value->setText(jsonObj["model"].toString());
        ui->number_cores_value->setText(QString::number(jsonObj["count"].toInt()));
    }

    reply->deleteLater();
}

void MainWindow::loadOs(QNetworkReply* reply)
{
    if(reply->error())
    {
        qDebug() << "ERROR!";
        qDebug() << reply->errorString();
    }
    else
    {
        QString strReply = (QString)reply->readAll();
        QJsonDocument jsonResponse = QJsonDocument::fromJson(strReply.toUtf8());
        QJsonObject jsonObj = jsonResponse.object();
        ui->architecture_value->setText(QString::number(jsonObj["architecture"].toDouble()));
        ui->os_value->setText(jsonObj["name"].toString());
    }

    reply->deleteLater();
}

void MainWindow::loadRam(QNetworkReply* reply)
{
    if(reply->error())
    {
        qDebug() << "ERROR!";
        qDebug() << reply->errorString();
    }
    else
    {
        QString strReply = (QString)reply->readAll();
        QJsonDocument jsonResponse = QJsonDocument::fromJson(strReply.toUtf8());
        QJsonObject jsonObj = jsonResponse.object();
        ui->ram_tot_value->setText(QString::number(jsonObj["total"].toInt()));
        ui->ram_free_value->setText(QString::number(jsonObj["free"].toInt()));
        ui->ram_used_value->setText(QString::number(jsonObj["usage"].toInt()));

        QDateTime date = QDateTime::currentDateTime();
        QJsonObject data;
        data["date"] = {date.toString("dd/MM/YYYY")};
        data["time"] = {date.toString("HH:mm:ss")};
        data["total"] = jsonObj["total"].toInt();
        data["free"] = jsonObj["free"].toInt();
        data["usage"] = jsonObj["usage"].toInt();
        dataRam[date.toString("dd/MM/YYYY HH:mm:ss")] = data;

        this->updateChart();
    }

    reply->deleteLater();
}

void MainWindow::loadHdd(QNetworkReply* reply)
{
    if(reply->error())
    {
        qDebug() << "ERROR!";
        qDebug() << reply->errorString();
    }
    else
    {
        QString strReply = (QString)reply->readAll();
        QJsonDocument jsonResponse = QJsonDocument::fromJson(strReply.toUtf8());
        QJsonObject jsonObj = jsonResponse.object();
        ui->hdd_total_value->setText(QString::number(jsonObj["total"].toVariant().toLongLong()));
        ui->hdd_usage_value->setText(QString::number(jsonObj["usage"].toVariant().toLongLong()));
        QJsonArray jsonArray = jsonObj["devices"].toArray();
        foreach (const QJsonValue & value, jsonArray) {
            QJsonObject obj = value.toObject();
            ui->hdd_name_value->setText(ui->hdd_name_value->text() + obj["name"].toString());
            //if (value == jsonArray.last()) {
            //   ui->hdd_name_value->setText(ui->hdd_name_value->text() + ", ");
            //}
        }
    }

    reply->deleteLater();
}

void MainWindow::updateChart()
{
    chartView->chart()->removeAllSeries();

    int x = 0;
    QLineSeries *new_series = new QLineSeries();

    foreach (const QJsonValue write, dataRam){
       new_series->append(x, write.toObject()["free"].toInt());
       x = x + (this->ms_time / 1000);
    }

    chartView->chart()->addSeries(new_series);
    chartView->chart()->createDefaultAxes();
    chartView->chart()->axisX()->setTitleText("Secondes");
    chartView->chart()->axisY()->setTitleText("Octets");
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_actionSetIp_triggered()
{
    QString new_url_api = QInputDialog::getText(this, "URL de l'API", "Quelle est l'URL de l'API ?", QLineEdit::Normal, this->url_api);
    if (new_url_api != "") {
        this->url_api = new_url_api;
        this->saveSettings();
        this->initTab();
    }
}

void MainWindow::on_actionExportData_triggered()
{
    QString filename = QFileDialog::getSaveFileName(this, "Exporter les données", "export.csv", "CSV files (.csv)", 0, 0);
    QFile data(filename);
    if(data.open(QFile::WriteOnly |QFile::Truncate))
    {
        QTextStream output(&data);
        output << "architecture;"<< ui->architecture_value->text() << endl;
        output << "coeurs;"<< ui->number_cores_value->text() << endl;
        output << "systeme d'exploitation;"<< ui->os_value->text() << endl;
        output << "model;"<< ui->cpu_value->text() << endl;
        output << "RAM totale;"<< ui->ram_tot_value->text() << endl;
        output << "RAM disponible;"<< ui->ram_free_value->text() << endl;
        output << "RAM utilisee;"<< ui->ram_used_value->text();
    }
}
