#include "util.h"
#include "lua.hpp"


extern "C" {
    int luaopen_sgs(lua_State *);
}

QVariant GetValueFromLuaState(lua_State *L, const char *table_name, const char *key)
{
    lua_getglobal(L, table_name);
    lua_getfield(L, -1, key);

    QVariant data;
    switch (lua_type(L, -1)) {
    case LUA_TSTRING: {
        data = QString::fromUtf8(lua_tostring(L, -1));
        lua_pop(L, 1);
        break;
    }
    case LUA_TNUMBER: {
        data = lua_tonumber(L, -1);
        lua_pop(L, 1);
        break;
    }
    case LUA_TTABLE: {
        lua_rawgeti(L, -1, 1);
        bool isArray = !lua_isnil(L, -1);
        lua_pop(L, 1);

        if (isArray) {
            QStringList list;

            size_t size = lua_rawlen(L, -1);
            for (size_t i = 0; i < size; i++) {
                lua_rawgeti(L, -1, i + 1);
                QString element = QString::fromUtf8(lua_tostring(L, -1));
                lua_pop(L, 1);
                list << element;
            }
            data = list;
        } else {
            QVariantMap map;
            int t = lua_gettop(L);
            for (lua_pushnil(L); lua_next(L, t); lua_pop(L, 1)) {
                const char *key = lua_tostring(L, -2);
                const char *value = lua_tostring(L, -1);
                map[key] = value;
            }
            data = map;
        }
    }
    default:
        break;
    }

    lua_pop(L, 1);
    return data;
}

lua_State *CreateLuaState()
{
    lua_State *L = luaL_newstate();
    luaL_openlibs(L);
    luaopen_sgs(L);

    return L;
}

bool DoLuaScript(lua_State *L, const char *script)
{
    int error = luaL_dofile(L, script);
    if (error) {
        QString error_msg = lua_tostring(L, -1);
        QMessageBox::critical(NULL, QObject::tr("Lua script error"), error_msg);
        return false;
    }
    return true;
}

QStringList IntList2StringList(const QList<int> &intlist)
{
    QStringList stringlist;
    for (int i = 0; i < intlist.size(); i++)
        stringlist.append(QString::number(intlist.at(i)));
    return stringlist;
}

QList<int> StringList2IntList(const QStringList &stringlist)
{
    QList<int> intlist;
    for (int i = 0; i < stringlist.size(); i++) {
        QString n = stringlist.at(i);
        bool ok;
        intlist.append(n.toInt(&ok));
        if (!ok) return QList<int>();
    }
    return intlist;
}

QVariantList IntList2VariantList(const QList<int> &intlist)
{
    QVariantList variantlist;
    for (int i = 0; i < intlist.size(); i++)
        variantlist.append(QVariant(intlist.at(i)));
    return variantlist;
}

QList<int> VariantList2IntList(const QVariantList &variantlist)
{
    QList<int> intlist;
    for (int i = 0; i < variantlist.size(); i++) {
        QVariant n = variantlist.at(i);
        bool ok;
        intlist.append(n.toInt(&ok));
        if (!ok) return QList<int>();
    }
    return intlist;
}

bool isNormalGameMode(const QString &mode)
{
    QRegExp moderx("(0[2-9]|10)p[dz]*");
    return moderx.exactMatch(mode);
}


QString IntList2String(const QList<int> &intlist)
{
    if (intlist.length() < 1)
        return QString();
    QString str(QString::number(intlist.at(0)));
    for (int i = 1; i < intlist.size(); i++)
        str.append('+' + QString::number(intlist.at(i)));
    return str;
}


QList<int> String2IntList(const QString &str)
{
    QList<int> intlist;
    if (str.isEmpty())
        return intlist;
    QStringList stringlist = str.split('+');
    for (int i = 0; i < stringlist.size(); i++) {
        QString n = stringlist.at(i);
        bool ok;
        intlist.append(n.toInt(&ok));
        if (!ok) return QList<int>();
    }
    return intlist;
}
