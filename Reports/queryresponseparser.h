//sgn
#ifndef QUERYRESPONSEPARSER_H
#define QUERYRESPONSEPARSER_H

#include <memory>
#include <QStringList>
#include <QVector>
#include <QTableWidgetItem>
#include "nlohmann_json.hpp"

using json = nlohmann::ordered_json;

class QueryResponseParser
{
    json mRoot;
    int32_t mRows, mCols;
public:
    typedef std::shared_ptr<QueryResponseParser> Ptr;
    QueryResponseParser(const json& pRoot)
        : mRoot(pRoot)
        , mRows(0)
        , mCols(0) {
            if(isOk()) {
                mRows = (int32_t)mRoot["rows"].size();
                mCols = (int32_t)mRoot["rows"][0].size();
            }
    }
    virtual ~QueryResponseParser() {}

    bool isOk() {
        return  mRoot.contains("rows") &&
                mRoot["rows"].is_array() &&
                (mRoot["rows"].size() > 0);
    }
    QStringList                 getColumnNames();
    QVector<QTableWidgetItem*>  getFieldsAsWidgetItems();
    QVector<QTableWidgetItem*>  getKeysAsWidgetItems();
    QString                     getQueryValue(int32_t iRow, int32_t iCol);
    int32_t                     getNoOfRows() { return mRows; }
    int32_t                     getNoOfCols() { return mCols; }
};


#endif // QUERYRESPONSEPARSER_H
