#ifndef MAINWINDOW_CALTEFE_H
#define MAINWINDOW_CALTEFE_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow_caltefe; }
QT_END_NAMESPACE

class MainWindow_caltefe : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow_caltefe(QWidget *parent = nullptr);
    ~MainWindow_caltefe();

private slots:
    void on_comboBox_espessura_currentIndexChanged();
    void on_pushButton_incluir_clicked();

private:
    Ui::MainWindow_caltefe *ui;
    int parameter;
    int thickness;
    int finalDiameter;

    void deleteSelectedRow();
    void createDB();
    void checkDB();
    void updateParameters();
    void loadThicknessValues();
    void loadFinalHoleValues(int thickness);
    QString loadGuideHoleValues(QString finalDiameter, QString thickness);
    void existGuideHole();
    float calcHolesPerHour(QString thickness, QString holeDiameter, int qtdFuros);
    QString convertDecimalTimeToString(double decimalTime);
    void finishingTime();
    void calcTotalHour();
    void clearWidgets();
    void includeValuesToTable(QString thickness, QString holeDiameter, int qtdFuros);
    void showAboutWindow();

};
#endif // MAINWINDOW_CALTEFE_H
