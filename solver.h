#ifndef SOLVER_H
#define SOLVER_H

#include <QList>
#include <QChar>
#include "trie.h"


class Solver
{
private:
    QList<QList<QChar>> board;
    Trie dictionary;
    QList<QString> solution;
public:
    Solver();
    void SetBoard(const QList<QList<QChar>> &newBoard);
    void LoadDictionary(QString path);
    void Solve();
    QList<QString> GetSolution();
    bool WordInDictionary(QString word);
private:
    void BoardTraverse(int row, int col, const QList<QList<QChar>>& tempBoard, const QString& word);
};

#endif // SOLVER_H
