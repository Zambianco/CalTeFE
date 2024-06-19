#include <QtSql>
#include <QMessageBox>
#include <QCoreApplication>
#include <QDebug>
#include <QTime>
#include <iomanip>
#include <QLocale>
#include <QTextStream>
#include <QShortcut>
#include <QtMath>

#include "mainwindow_caltefe.h"
#include "ui_mainwindow_caltefe.h"

// Variável global para o tempo padrão de acabamento para cada furo
double FINISHING_TIME = 0.002777777777777; // 10 segundos

MainWindow_caltefe::MainWindow_caltefe(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow_caltefe)
{
    ui->setupUi(this);

    // >>>>> CONFIGURAÇÃO INCIAL >>>>>
    MainWindow_caltefe::checkDB();
    MainWindow_caltefe::loadThicknessValues();
    MainWindow_caltefe::existGuideHole();
    // <<<<< CONFIGURAÇÃO INCIAL <<<<<


    // Instala um evento filter para capturar o pressionamento de teclas
    this->installEventFilter(this);


    // >>>>> CONNECTS >>>>>
    // Reação quando é selecionada uma outra espessura
    connect(ui->comboBox_espessura, SIGNAL(currentIndexChanged(int)), this, SLOT(on_comboBox_espessura_currentIndexChanged()));

    // Reação quando é selecionado um outro diâmetro de furo
    connect(ui->comboBox_diamFuro, &QComboBox::currentIndexChanged, this, &MainWindow_caltefe::existGuideHole);

    // Quando a quantidade de furos é altera um novo tempo de acabamento é calculado
    connect(ui->spinBox_qtdFuros, &QSpinBox::valueChanged, this, &MainWindow_caltefe::finishingTime);

    // Quando a tecla DELETE é pressionada
    QShortcut *keypress_delete = new QShortcut(QKeySequence(Qt::Key_Delete), this);
    connect(keypress_delete, &QShortcut::activated, this, &MainWindow_caltefe::deleteSelectedRow);

    // Quando a tecla ESCAPE é pressionada
    QShortcut *keypress_escape = new QShortcut(QKeySequence(Qt::Key_Escape), this);
    connect(keypress_escape, &QShortcut::activated, this, &MainWindow_caltefe::clearWidgets);


    // Botão "SOBRE"
    connect(ui->actionAbout, &QAction::triggered, this, &MainWindow_caltefe::showAboutWindow);
    // <<<<< CONNECTS <<<<<
}

// Função para mostrar a janela de informações sobre o programa
void MainWindow_caltefe::showAboutWindow() {
    QMessageBox aboutBox;
    aboutBox.setWindowTitle("Sobre");
    aboutBox.setText("Aplicação de cálculo de tempo de furação de espelho\n"
                     "Desenvolvido por Veloster\n"
                     "Versão: 0.1.0");

    aboutBox.exec();
}


// Função para reiniciar os Widgets da janela
void MainWindow_caltefe::clearWidgets()
{
    // ---------------------
    // SPINBOX spinBox_qtdFuros
    // Reseta o valor dos widgets
    ui->spinBox_qtdFuros->setValue(0);
    ui->spinBox_qtdFuros->setEnabled(true);
    // ---------------------

    // ---------------------
    // COMBOBOX comboBox_guia
    ui->comboBox_guia->setCurrentIndex(0);
    ui->comboBox_guia->setEnabled(true);
    // ---------------------

    // ---------------------
    // LABEL label_tempoTotal
    // Atualiza o tempo total para exibir 0
    ui->label_tempoTotal->setText("----");
    // ---------------------

    // ---------------------
    // COMBOBOX comboBox_espessura
    // Atualiza a espessura para exibir o primeiro item (supondo que o índice 0 é o valor padrão)
    ui->comboBox_espessura->setCurrentIndex(0);
    ui->comboBox_espessura->setEnabled(true);
    // ---------------------

    // ---------------------
    // TABELA tableWidget_duration
    ui->tableWidget_duration->clearContents(); // Limpa o conteúdo da tabela
    ui->tableWidget_duration->setCurrentCell(-1, -1); // Zera o índice da linha selecionada
    ui->tableWidget_duration->setRowCount(0);
    // ---------------------

    // Refaz a configuração inicial
    // >>>>> CONFIGURAÇÃO INCIAL >>>>>
    MainWindow_caltefe::loadThicknessValues();
    MainWindow_caltefe::existGuideHole();
    // <<<<< CONFIGURAÇÃO INCIAL <<<<<

    ui->statusbar->showMessage("Widget resetados", 3000);
}


// Função para excluir a linha selecionada na tabela
void MainWindow_caltefe::deleteSelectedRow()
{
    // excluí a linha selecionada na tabela
    int selectedRow = ui->tableWidget_duration->currentRow();

    // Caso a linha selecionada seja a de tempo de acabamento ela não será excluída
    // somente são excluída linhas para outras tarefas
    QTableWidgetItem *item = ui->tableWidget_duration->item(selectedRow, 0); // Obtém o item da coluna 0 da linha atual
    if (item && item->text() == "Acabamento") {
        ui->statusbar->showMessage("Não é possível excluir o acabamento", 3000);
    } else {
        ui->tableWidget_duration->removeRow(selectedRow);
        ui->statusbar->showMessage("Linha excluída", 3000);
        finishingTime();
        calcTotalHour();
    }
}


MainWindow_caltefe::~MainWindow_caltefe()
{
    delete ui;
}


// Função para criar o banco de dados
void MainWindow_caltefe::createDB(){
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("parameters.db");

    // Verifica se a conexão foi bem sucedida
    if (!db.open()) {
        qDebug() << "Erro ao abrir o banco de dados";
        return;
    } else {
        QMessageBox msgBox;
        msgBox.setText("Conexão ao banco de dados realizada");
        msgBox.exec();
    }

    // Criação da tabela gi_tempopadrao_furadeira
    QSqlQuery query;
    query.prepare("CREATE TABLE IF NOT EXISTS parameters_drill ("
                  "ID_drill_parameter INTEGER PRIMARY KEY AUTOINCREMENT UNIQUE NOT NULL,"
                  "thickness          INTEGER NOT NULL,"
                  "hole_diameter      TEXT    NOT NULL,"
                  "hole_per_hour      INTEGER,"
                  "guide              INTEGER REFERENCES parameters_drill (ID_drill_parameter) ON DELETE RESTRICT"
                  "    ON UPDATE CASCADE"
                  ")"
                  "STRICT;");

    if (!query.exec()) {
        QMessageBox msgBox;
        msgBox.setText("Erro ao criar a tabela parameters_drill\n"
                       + query.lastError().text());
        msgBox.exec();
        return;
    }
}


// Função para verificar se o banco de dados existe
void MainWindow_caltefe::checkDB(){
    // Verifica se o banco de dados existe
    QSqlDatabase db = QSqlDatabase::database();

    if (!db.isOpen()) {
        // Se o banco de dados não estiver aberto, chama a função createDB para criá-lo
        createDB();
        ui->statusbar->showMessage("Banco de dados criado", 3000);
    } else {
        // Verifica se o banco de dados está conectado
        if (!db.isValid()) {
            QMessageBox msgBox;
            msgBox.setText("Erro: o banco de dados não é válido.\n");
            return;
        }

        // Verifica se a tabela existe no banco de dados
        if (!db.tables().contains("parameters_drill")) {
            // Se a tabela não existir, chama a função createDB para criá-la
            createDB();
            ui->statusbar->showMessage("Tabela antes inexistente, agora criada", 3000);
        } else {
            // Exibe uma mensagem indicando que o banco de dados está OK na statusbar
            ui->statusbar->showMessage("Banco de dados OK", 3000);
        }
    }
}


// Função para atualizar os parâmetros de corte de dados disponibilizados online
void MainWindow_caltefe::updateParameters(){


}


// Função para calcular o tempo de acabamento para a quantidade de furos selecionada
void MainWindow_caltefe::finishingTime(){

    int qtdFuros = ui->spinBox_qtdFuros->value();

    // Calcula o tempo de acabamento
    double finishingTime = qtdFuros * 4 * FINISHING_TIME; // Supondo que FINISHING_TIME é uma constante definida

    // Formatação do número para exibição
    QString formattedFinishingTime = QString::number(finishingTime, 'f', 2);
    formattedFinishingTime.replace('.', ',');

    // Verifica se já existe a linha com o texto "Acabamento"
    bool found = false;
    int rowCount = ui->tableWidget_duration->rowCount();
    for (int row = 0; row < rowCount; ++row) {
        QTableWidgetItem *item = ui->tableWidget_duration->item(row, 0); // Obtém o item da coluna 0 da linha atual
        if (item && item->text() == "Acabamento") {
            // Atualiza o valor da coluna 1
            ui->tableWidget_duration->item(row, 1)->setText(formattedFinishingTime);
            found = true;
            break;
        }
    }

    // Se não encontrou a linha com o texto "Acabamento", então adiciona uma nova linha
    if (!found) {
        int row = ui->tableWidget_duration->rowCount(); // Obtém a próxima linha disponível
        ui->tableWidget_duration->insertRow(row); // Insere uma nova linha
        // Preenche as colunas da nova linha
        QTableWidgetItem *item1 = new QTableWidgetItem("Acabamento");
        QTableWidgetItem *item2 = new QTableWidgetItem(formattedFinishingTime);
        ui->tableWidget_duration->setItem(row, 0, item1); // Coluna 0: Acabamento
        ui->tableWidget_duration->setItem(row, 1, item2); // Coluna 1: formattedFinishingTime

        item2->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    }
}


// Função para verificar se há furo guia para a espessura e diâmetro de furo selecionados
void MainWindow_caltefe::existGuideHole(){
    QString thickness = ui->comboBox_espessura->currentText();
    QString holeDiameter = ui->comboBox_diamFuro->currentText();

    // Conexão ao banco de dados
    QSqlDatabase db = QSqlDatabase::database();

    // Verifica se o banco de dados está aberto
    if (!db.isOpen()) {
        QMessageBox msgBox;
        msgBox.setText("Erro: o banco de dados não está aberto.\n");
        return;
    }

    // Prepara a consulta SQL
    QSqlQuery query;
    query.prepare("SELECT guide FROM parameters_drill "
                  "WHERE thickness = :thickness "
                    "AND hole_diameter = :holeDiameter");
    query.bindValue(":thickness", thickness);
    query.bindValue(":holeDiameter", holeDiameter);

    // Executa a consulta
    if (!query.exec()) {
        QMessageBox msgBox;
        msgBox.setText("Erro ao executar a consulta:\n"
                       + query.lastError().text());
        msgBox.exec();
        return;
    }

    // Verifica se há resultados
    if (query.next()) {
        // Obtém o valor de guide
        QVariant guideValue = query.value(0);
        // Verifica se o valor é nulo
        if (guideValue.isNull()) {
            ui->comboBox_guia->setEnabled(false);
            ui->comboBox_guia->setCurrentText("Não");
        } else {
            ui->comboBox_guia->setEnabled(true);
        }
    } else {
        QMessageBox msgBox;
        msgBox.setText("Erro: Não há entradas para a espessura selecionada\n"
                       + query.lastError().text());
    }
}


// Função para carregar os valores de diâmetro final de furo
void MainWindow_caltefe::loadFinalHoleValues(int thickness) {
    // Conexão ao banco de dados
    QSqlDatabase db = QSqlDatabase::database();

    // Verifica se o banco de dados está aberto
    if (!db.isOpen()) {
        qDebug() << "Erro: o banco de dados não está aberto.";
        return;
    }

    // Prepara a consulta SQL
    QSqlQuery query;
    query.prepare("SELECT hole_diameter, guide "
                  "FROM parameters_drill "
                  "WHERE thickness = :thickness");
    query.bindValue(":thickness", thickness);

    // Executa a consulta
    if (!query.exec()) {
        qDebug() << "Erro ao executar a consulta:" << query.lastError().text();
        return;
    }

    // Limpa os itens existentes no comboBox_final
    ui->comboBox_diamFuro->clear();

    // Processa os resultados da consulta
    while (query.next()) {
        QString holeDiameter = query.value(0).toString();
        // Adiciona cada valor de hole_diameter como um item no comboBox_final
        ui->comboBox_diamFuro->addItem(holeDiameter);

    }

}


// Função para carregar os valores de espessura do espelho atualmente existentes no banco de dados
void MainWindow_caltefe::loadThicknessValues() {
    //Carregao os valores de espessura do espelho atualmente
    //existentes no banco de dados

    // Conexão ao banco de dados
    QSqlDatabase db = QSqlDatabase::database();

    // Verifica se o banco de dados está aberto
    if (!db.isOpen()) {
        qDebug() << "Erro: o banco de dados não está aberto.";
        return;
    }

    // Prepara a consulta SQL para obter valores distintos da coluna thickness
    QSqlQuery query;
    query.prepare("SELECT DISTINCT thickness "
                  "FROM parameters_drill "
                  "ORDER BY thickness ASC");

    // Executa a consulta
    if (!query.exec()) {
        qDebug() << "Erro ao executar a consulta:"
                 << query.lastError().text();
        return;
    }

    // Limpa os itens existentes no comboBox_espessura
    ui->comboBox_espessura->clear();

    // Processa os resultados da consulta
    while (query.next()) {
        int thicknessValue = query.value(0).toInt();
        // Adiciona o valor de thickness como um item no comboBox_espessura
        ui->comboBox_espessura->addItem(QString::number(thicknessValue));
    }

}


// Função para carregar os valores de espessura do espelho atualmente existentes no banco de dados
void MainWindow_caltefe::on_comboBox_espessura_currentIndexChanged()
{
    // Obtém o texto do item selecionado no comboBox_espessura
    QString selectedThickness = ui->comboBox_espessura->currentText();

    // Converte o texto para um valor inteiro, se necessário
    int selectedThicknessValue = selectedThickness.toInt();

    loadFinalHoleValues(selectedThicknessValue);
}


// Função para calcular a quantidade de furos por hora
float MainWindow_caltefe::calcHolesPerHour(QString thickness, QString holeDiameter, int qtdFuros){
    // Abra a conexão com o banco de dados
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("parameters.db");

    if (!db.open()) {
        qDebug() << "Erro ao abrir o banco de dados";
        return -1;
    }

    // Consulta SQL para obter o valor de hole_per_hour
    QSqlQuery query;
    query.prepare("SELECT hole_per_hour FROM parameters_drill WHERE thickness = :thickness AND hole_diameter = :holeDiameter");
    query.bindValue(":thickness", thickness);
    query.bindValue(":holeDiameter", holeDiameter);

    if (!query.exec()) {
        qDebug() << "Erro ao executar a consulta SQL:" << query.lastError().text();
        db.close();
        return -1;
    }

    // Verifica se a consulta retornou algum resultado
    if (query.next()) {
        // Obtem o valor de hole_per_hour
        int holePerHour = query.value(0).toInt();

        // Realiza a operação qtdFuros/holePerHour
        double tempoEstimado = static_cast<double>(qtdFuros) / holePerHour;

        // Limita o resultado a duas casas decimais
        tempoEstimado = std::round(tempoEstimado * 100) / 100;

        return tempoEstimado;
    } else {
        qDebug() << "Nenhum valor encontrado para os parâmetros fornecidos";
    }

    // Feche a conexão com o banco de dados
    db.close();
}


// Função para converter o tempo decimal em uma string formatada (hh:mm)
QString MainWindow_caltefe::convertDecimalTimeToString(double decimalTime) {
    int horas = static_cast<int>(decimalTime);  // Obtém a parte inteira da hora decimal
    int minutos = static_cast<int>((decimalTime - horas) * 60);  // Calcula os minutos

    std::ostringstream ss;
    ss << std::setw(2) << std::setfill('0') << horas << ":" << std::setw(2) << std::setfill('0') << minutos;
    return QString::fromStdString(ss.str());
}


// Função para calcular o total de horas e atualizar o texto do label_tempoTotal
void MainWindow_caltefe::calcTotalHour() {
    // Inicializa a soma como zero
    float totalHour = 0.0;

    // Itera sobre todas as linhas da tabela somando as horas
    for (int i = 0; i < ui->tableWidget_duration->rowCount(); ++i) {
        QTableWidgetItem *item = ui->tableWidget_duration->item(i, 1); // Coluna 1 (0-indexed)
        if (item) {
            totalHour += item->text().replace(",", ".").toFloat();
        }
    }

    // Arredonda totalHour para cima
    float roundedTotalHour = std::ceil(totalHour);

    // Converte o valor arredondado para QString
    QString formattedTotalHour = QString::number(roundedTotalHour) + " horas";

    // Atualiza o texto do label_tempoTotal
    ui->label_tempoTotal->setText(formattedTotalHour);


    //-------------------------
    // Calcula a quantidade de dias aproximada
    float days = roundedTotalHour / 8.8;

    // Arredonda para cima e limita a nenhuma casa decimal
    int roundedDays = qCeil(days);

    // Converte o valor arredondado para QString
    QString formattedTotaldays = "~ " + QString::number(roundedDays) + " dias";

    // Atualiza o texto do label_tempoTotalDias
    ui->label_tempoTotalDias->setText(formattedTotaldays);
    //-------------------------

}


// Função para buscar as informações do furo guia
QString MainWindow_caltefe::loadGuideHoleValues(QString finalDiameter, QString thickness) {
    // Conexão ao banco de dados
    QSqlDatabase db = QSqlDatabase::database();

    // Verifica se o banco de dados está aberto
    if (!db.isOpen()) {
        QMessageBox msgBox;
        msgBox.setText("Erro: o banco de dados não está aberto.\n");
        msgBox.exec();
        return "";  // Retorna uma string vazia indicando erro
    }

    // Prepara a consulta SQL para obter o guide
    QSqlQuery query;
    query.prepare("SELECT guide FROM parameters_drill "
                  "WHERE thickness = :thickness "
                  "AND hole_diameter = :holeDiameter");
    query.bindValue(":thickness", thickness);
    query.bindValue(":holeDiameter", finalDiameter);

    // Executa a consulta
    if (!query.exec()) {
        QMessageBox msgBox;
        msgBox.setText("Erro ao executar a consulta para obter o guide:\n"
                       + query.lastError().text());
        msgBox.exec();
        return "";  // Retorna uma string vazia indicando erro
    }

    // Verifica se houve resultado na consulta
    if (!query.next()) {
        QMessageBox msgBox;
        msgBox.setText("Nenhum guide encontrado para os parâmetros fornecidos.\n");
        msgBox.exec();
        return "";  // Retorna uma string vazia indicando erro
    }

    int guide = query.value(0).toInt();

    // Prepara a consulta SQL para obter o hole_diameter usando o guide encontrado
    query.prepare("SELECT hole_diameter FROM parameters_drill WHERE ID_drill_parameter = :guide");
    query.bindValue(":guide", guide);

    // Executa a consulta
    if (!query.exec()) {
        QMessageBox msgBox;
        msgBox.setText("Erro ao executar a consulta para obter hole_diameter:\n"
                       + query.lastError().text());
        msgBox.exec();
        return "";  // Retorna uma string vazia indicando erro
    }

    // Verifica se houve resultado na consulta
    if (!query.next()) {
        QMessageBox msgBox;
        msgBox.setText("Nenhum hole_diameter encontrado para o guia fornecido.\n");
        msgBox.exec();
        return "";  // Retorna uma string vazia indicando erro
    }

    QString holeDiameter = query.value(0).toString();

    // Retorna o valor de hole_diameter encontrado
    return holeDiameter;
}


// Função para incluir os valores na tabela
void MainWindow_caltefe::includeValuesToTable(QString thickness, QString holeDiameter, int qtdFuros){
    float timeDrill;
    timeDrill = MainWindow_caltefe::calcHolesPerHour(thickness,
                                                     holeDiameter,
                                                     qtdFuros);

    // Formata o número para sempre ter duas casas decimais
    QString formattedTimeDrill = QString::number(timeDrill, 'f', 2);
    // Substitui o ponto pela vírgula
    formattedTimeDrill.replace('.', ',');

    // >>>>> Adicionar os valores na tabela >>>>>
    int row = ui->tableWidget_duration->rowCount(); // Obtém a próxima linha disponível
    ui->tableWidget_duration->insertRow(row); // Insere uma nova linha

    // Preenche as colunas da nova linha
    QTableWidgetItem *item1 = new QTableWidgetItem(holeDiameter);
    QTableWidgetItem *item2 = new QTableWidgetItem(formattedTimeDrill);
    ui->tableWidget_duration->setItem(row, 0, item1); // Coluna 0: holeDiameter
    ui->tableWidget_duration->setItem(row, 1, item2); // Coluna 1: holesPerHour

    // Define o alinhamento horizontal para a coluna 1 como alinhamento à direita
    item2->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);

    // Ajusta a largura da coluna 0 ao conteúdo
    ui->tableWidget_duration->resizeColumnToContents(0);
    // <<<<< Adicionar os valores na tabela <<<<<

}

// Slot para o botão de inclusão
void MainWindow_caltefe::on_pushButton_incluir_clicked()
{
    QLocale::setDefault(QLocale(QLocale::Portuguese, QLocale::Brazil));

    // Para evitar que o usuário altere valores de espessura e quantidade os
    //widgets responsáveis pela seleção destes valores são desabilitados
    ui->comboBox_espessura->setEnabled(false);
    ui->spinBox_qtdFuros->setEnabled(false);

    // Obtenção dos valores dos widgets da janela
    QString thickness = ui->comboBox_espessura->currentText();
    int qtdFuros = ui->spinBox_qtdFuros->value();
    QString holeDiameter = ui->comboBox_diamFuro->currentText();
    QString guide = ui->comboBox_guia->currentText();

    // Obtem o valor de furos por hora e preenche a tabela com o tempo
    includeValuesToTable(thickness,
                         holeDiameter,
                         qtdFuros);

    // Se estiver seleciondo para incluir furo guia
    if (guide=="Sim"){
        QString guideHole = loadGuideHoleValues(holeDiameter, thickness);

        includeValuesToTable(thickness,
                             guideHole,
                             qtdFuros);
    }

    calcTotalHour();


}

