//sgn
#include <QString>
#include <QDate>
#include "queryresponseparser.h"
#include "nlohmann_json.hpp"

using json = nlohmann::ordered_json;

QStringList QueryResponseParser::getColumnNames() {
    QStringList columnNames;
    if(!isOk()) return columnNames;

    json pRow = mRoot["rows"][0];
    for(auto itr = pRow.begin(); itr != pRow.end(); itr++) {
        columnNames << itr.key().c_str();
    }
    return columnNames;
}

QVector<QTableWidgetItem*> QueryResponseParser::getFieldsAsWidgetItems() {
    QVector<QTableWidgetItem*> pItems;
    std::string strKey, strVal;
    int32_t     iVal = 0;
    float       fVal = 0.0;
    bool        bVal = false;
    QString     qstrKey, qstrVal;
    if(!isOk()) return pItems;

    QTableWidgetItem* pItem = nullptr;
    json pRows              = mRoot["rows"];
    for(const auto& pRow : pRows) {
        for(auto& col : pRow.items()) {
            pItem   = new QTableWidgetItem();
            strKey  = col.key();
            qstrKey = strKey.c_str();

            switch(col.value().type()) {
            case nlohmann::detail::value_t::string:
                strVal  = col.value();
                qstrVal = strVal.c_str();
                if(qstrKey.contains("validity", Qt::CaseInsensitive) || qstrKey.contains("expiry", Qt::CaseInsensitive)) {
                    QDate pToday    = QDate::currentDate();
                    QDate pExpiry   = QDate::fromString(qstrVal, "dd-MM-yyyy");
                    if(pExpiry < pToday) {
                        pItem->setForeground(QBrush(QBrush(QColor(255, 255, 255))));
                        pItem->setBackground(QBrush(QBrush(QColor(255, 0, 0))));
                    }
                }
                pItem->setText(strVal.c_str());
                break;
            case nlohmann::detail::value_t::number_integer:
            case nlohmann::detail::value_t::number_unsigned:
                iVal    = col.value();
                pItem->setData(Qt::DisplayRole, QVariant(iVal));
                break;
            case nlohmann::detail::value_t::number_float:
                fVal    = col.value();
                pItem->setData(Qt::DisplayRole, QVariant(fVal));
                break;
            case nlohmann::detail::value_t::boolean:
                bVal    = col.value();
                pItem->setData(Qt::DisplayRole, QVariant(bVal));
                break;
            default:
                delete pItem; pItem = nullptr;
                break;
            }
            if(pItem) pItems.push_back(pItem);
        }
    }
    return pItems;
}

QVector<QTableWidgetItem*> QueryResponseParser::getKeysAsWidgetItems() {
    QVector<QTableWidgetItem*> pItems;
    if(!isOk()) return pItems;

    std::string strVal;
    QTableWidgetItem* pItem = nullptr;
    json pRows              = mRoot["rows"];
    for(const auto& pRow : pRows) {
        for(auto& col : pRow.items()) {
            pItem   = new QTableWidgetItem();
            strVal  = col.key();
            pItem->setText(strVal.c_str());
            pItems.push_back(pItem);
        }
    }
    return pItems;
}

QString QueryResponseParser::getQueryValue(int32_t iRow, int32_t iCol) {
    if(!isOk() || iRow >= mRows || iCol >= mCols) return "";
    json pRows  = mRoot["rows"];
    json pRow   = pRows[iRow];

    int32_t iCount = 0;
    std::string strVal;
    for(auto& col : pRow.items()) if(iCol == iCount++) { strVal = col.value(); break; }
    return strVal.c_str();
}
