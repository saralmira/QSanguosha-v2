#ifndef _UTIL_H
#define _UTIL_H

struct lua_State;
class QVariant;

template<typename T>
void qShuffle(QList<T> &list)
{
    //qsrand(QTime(0,0,0).msecsTo(QTime::currentTime()));
    int i, n = list.length();
    for (i = 0; i < n; i++) {
        int r = qrand() % (n - i) + i;
        list.swap(i, r);
    }
}

inline const char* QString2CStr(const QString &str)
{
    QByteArray arr = str.toLocal8Bit();
    return arr.data();
}

// lua interpreter related
lua_State *CreateLuaState();
bool DoLuaScript(lua_State *L, const char *script);

QVariant GetValueFromLuaState(lua_State *L, const char *table_name, const char *key);

QStringList IntList2StringList(const QList<int> &intlist);
QList<int> StringList2IntList(const QStringList &stringlist);
QString IntList2String(const QList<int> &intlist);
QList<int> String2IntList(const QString &str);
QVariantList IntList2VariantList(const QList<int> &intlist);
QList<int> VariantList2IntList(const QVariantList &variantlist);

bool isNormalGameMode(const QString &mode);

static const int S_EQUIP_AREA_LENGTH = 5;

#endif

