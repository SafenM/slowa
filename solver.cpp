#include "solver.h"
#include <QFile>
#include <QDebug>
#include <QDir>

Solver::Solver()
{

}

void Solver::LoadDictionary(QString path)
{
    QFile file(path);
    if(!file.exists())
    {
        qDebug() << "File cannot be found" << path << " current path: " << QDir::currentPath();
    }
    else
    {
        qDebug() << path << " Opening...";
    }
    QString line;
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)){
        QTextStream stream(&file);
        while (!stream.atEnd()){
            line = stream.readLine();
            dictionary.Add(line);
        }
    }
    file.close();
}

void Solver::SetBoard(const QList<QList<QChar>> &newBoard)
{
    this->board = newBoard;
}

void Solver::Solve()
{
    solution.clear();
    for (int i = 0; i < board.size(); i++) {
        for (int j = 0; j < board[i].size(); j++) {
            QString emptyString{};
            BoardTraverse(i, j, board, emptyString);
        }
    }
}

void Solver::BoardTraverse(int row, int col, const QList<QList<QChar>>& tempBoard, const QString& word)
{
    if (row < 0  || col < 0 || row >= tempBoard.size() || col >= tempBoard[0].size())
    {
        return;
    }
    if (tempBoard[row][col].isNull())
    {
        return;
    }

    QString newWord = word + tempBoard[row][col];

    ETrieSearchResult result = dictionary.SearchForWord(newWord);
    if (result != ETrieSearchResult::NotFound)
    {
        if (result == ETrieSearchResult::WordFound)
        {
            solution.push_back(newWord);
        }

        QList<QList<QChar>> newBoard = tempBoard;
        newBoard[row][col] = '\0';

        BoardTraverse(row, col + 1, newBoard, newWord);
        BoardTraverse(row, col - 1, newBoard, newWord);
        BoardTraverse(row + 1, col, newBoard, newWord);
        BoardTraverse(row - 1, col, newBoard, newWord);
        BoardTraverse(row + 1, col - 1, newBoard, newWord);
        BoardTraverse(row + 1, col + 1, newBoard, newWord);
        BoardTraverse(row - 1, col - 1, newBoard, newWord);
        BoardTraverse(row - 1, col + 1, newBoard, newWord);
    }
}

QList<QString> Solver::GetSolution()
{
    return solution;
}

bool Solver::WordInDictionary(QString word)
{
    if (dictionary.SearchForWord(word) == ETrieSearchResult::WordFound)
    {
        return true;
    }
    return false;
}
