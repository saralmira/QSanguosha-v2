#ifndef _EXPPATTERN_H
#define _EXPPATTERN_H

#include "package.h"

// '|' means 'and', '#' means 'or'.
// the expression splited by '|' has 4 parts,
// 1st part means the card name, and ',' means more than one options.
// 2nd patt means the card suit, and ',' means more than one options.
// 3rd part means the card number, and ',' means more than one options,
// the number uses '~' to make a scale for valid expressions
// 4th part means the card place, and ',' means more than one options,
// "hand" stands for handcard and "equipped" stands for the cards in the placeequip
// if it is neigher "hand" nor "equipped", it stands for the pile the card is in.

class ExpPattern : public CardPattern
{
public:
    ExpPattern(const QString &exp);
    virtual bool match(const Player *player, const Card *card) const;
private:
    QString exp;
    bool matchOne(const Player *player, const Card *card, QString exp) const;
};

#endif

