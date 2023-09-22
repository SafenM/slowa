#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QMessageBox>
#include <QDebug>
#include <QPushButton>
#include <QTextEdit>
#include <QLayout>
#include <QRandomGenerator>
#include <algorithm>
#include "params.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    for (int i = 0; i < PARAMS::BOARD_SIZE; i++)
    {
        for (int j = 0; j < PARAMS::BOARD_SIZE; j++)
        {
            QTextEdit* textEdit = new QTextEdit;
            textEdit->setTabChangesFocus(true);
            textEdit->setFontPointSize(26);
            ui->gridLayout->addWidget(textEdit, i, j);
            letters.push_back(textEdit);
        }
    }
    mySolver.LoadDictionary(PARAMS::SlownikPATH);
}

MainWindow::~MainWindow()
{
    delete ui;
}

QList<QList<QChar>> MainWindow::getBoard()
{
    QList<QList<QChar>> result;
    for (int i = 0; i < PARAMS::BOARD_SIZE; i++)
    {
        result.push_back(QList<QChar>{});
        for (int j = 0; j < PARAMS::BOARD_SIZE; j++)
        {
            if (!letters[i*PARAMS::BOARD_SIZE + j]->toPlainText().isEmpty())
            {
                result[i].push_back(letters[i*PARAMS::BOARD_SIZE + j]->toPlainText()[0]);
            }
            else
            {
                result[i].push_back('\0');
            }
        }
    }
    return result;
}

void MainWindow::on_pushButton_clicked()
{
    mySolver.SetBoard(getBoard());
    qDebug() << "Solving";
    mySolver.Solve();
    qDebug() << "Finished solving";
    QList<QString> solution = mySolver.GetSolution();
    std::sort(solution.begin(), solution.end());
    QString result{};
    for (QString& word : solution)
    {
        result += word;
        result += "\n";
    }
    ui->plainTextEdit->setPlainText(result);
}


void MainWindow::on_pushButton_2_clicked()
{
    for (auto box : letters)
    {
        box->setPlainText(PARAMS::Alphabet[QRandomGenerator::global()->bounded(PARAMS::Alphabet.size())]);
    }
}

