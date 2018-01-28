#include <QCoreApplication>
#include <QSqlDatabase>
#include <QSqlDriver>
#include <QSqlQuery>
#include <QDebug>
#include <QSql>
#include "open62541.h"
#include <stdio.h>
#include <stdlib.h>
#include <QTime>

QSqlDatabase db=QSqlDatabase::addDatabase("QMYSQL");
int numRows;
QList<int> intlist;
QList<QString> stringlist;
QSqlQuery query(db);
UA_Client *client=NULL;
int debug=1;

void sleep(unsigned int msec) //延时函数
{
    //currentTime()返回当前时间，用当前时间加上我们要的延时时间msec得到一个新时刻
    QTime reachTime=QTime::currentTime().addMSecs(msec);
    while(QTime::currentTime()<reachTime)
        QCoreApplication::processEvents(QEventLoop::AllEvents,100);
}

int ConnectToDB()   //连接数据库
{
    db.setHostName("127.0.0.1");
    db.setDatabaseName("opc ua");
    db.setUserName("root");
    db.setPort(3306);
    db.setPassword("123456");
    if(!db.open()){
        return 0;
    }
    else
        return 1;

}
int ReadFromDB() //从数据库里取出要读的数据
{
    query.exec("select * from nodetoread");
    if(db.driver()->hasFeature(QSqlDriver::QuerySize)){
        qDebug()<<"has feature:query size:"<<query.size();
        numRows=query.size();
    }
    else
        return 0;
        while(query.next()){
        intlist.append(query.value(0).toInt());
        stringlist.append(query.value(1).toString());
    }

        qDebug()<<intlist;
        qDebug()<<stringlist;
    if(intlist.size()==0)
        return 0;
    else
        return 1;
}
int ReadFromServer()   //从OPC UA Server读取数据并写入到数据库
{
  int i=0;
  //query.exec("create table result(nodeid nvarchar(255),value nvarchar(255), datatype varchar(50))");
  query.prepare("insert into result (nodeid,value,datatype) values(:nodeid,:value,:datatype)");

  //连接OPC UA Server
  UA_Client *client = UA_Client_new(UA_ClientConfig_standard);
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:48010");
  if(retval != UA_STATUSCODE_GOOD)
  {
    UA_Client_delete(client);
   // return retval;
    return 0;
  }

  //新建并初始化ReadRequest
   UA_ReadRequest rReq;
   UA_ReadRequest_init(&rReq);
   void *idptr = UA_Array_new(numRows,&UA_TYPES[UA_TYPES_READVALUEID]);
   rReq.nodesToRead = (UA_ReadValueId*) idptr;
   rReq.nodesToReadSize = numRows;
   for(i=0;i<numRows;i++){
       if(debug){
           qDebug()<<"ns="<<intlist.at(i);
           qDebug()<<"name="<<stringlist.at(i);
       }
       //将QString类转化为const char*
       QByteArray ba=stringlist.at(i).toLatin1();
       const char* nodeid=NULL;
       nodeid=ba.data();

       if(isdigit(*nodeid)==0){  //这里只判断了nodeid所指向字符串的第一个字符,需要再看看,难道不会误报吗

        rReq.nodesToRead[i].nodeId = UA_NODEID_STRING_ALLOC(intlist.at(i),nodeid );// assume this node exists
        rReq.nodesToRead[i].attributeId = UA_ATTRIBUTEID_VALUE;
      }
      else // *type == 'N')
      {
        rReq.nodesToRead[i].nodeId = UA_NODEID_NUMERIC(intlist.at(i), atoi(nodeid)); // assume this node exists
        rReq.nodesToRead[i].attributeId = UA_ATTRIBUTEID_VALUE;
      }
    }
   while(1){
    UA_ReadResponse rResp = UA_Client_Service_read(client, rReq);
    if(rResp.responseHeader.serviceResult == UA_STATUSCODE_GOOD && rResp.resultsSize > 0){
        for(i=0;i<numRows;i++){
            if(debug){
          printf("rResp.results[%d].hasValue=%d\n",i,rResp.results[i].hasValue);
          printf("UA_Variant_isScalar(&rResp.results[%d].value=%d\n",i,UA_Variant_isScalar(&rResp.results[i].value));
            }


       QByteArray ba=stringlist.at(i).toLatin1();
       const char* nodeid=NULL;     //与上面的for循环中的nodeid不是一个
       nodeid=ba.data();
       qDebug()<<nodeid;

            if(rResp.results[i].hasValue && UA_Variant_isScalar(&rResp.results[i].value))
            {
              if(debug) printf("rResp.results[%d].value.type=%ld\n",i,(long) rResp.results[i].value.type);
              if(rResp.results[i].value.type == &UA_TYPES[UA_TYPES_BOOLEAN])
              {
                int val = *(UA_Boolean*)rResp.results[i].value.data;
                if(debug) printf("the value is: Boolean=%d\n", val);
                query.bindValue(":value",val);
                query.bindValue(":datatype","Boolean");
                query.bindValue(":nodeid",nodeid);
                query.exec();
              }
              else if(rResp.results[i].value.type == &UA_TYPES[UA_TYPES_SBYTE])
              {
                int val = *(UA_SByte*)rResp.results[i].value.data;
                if(debug) printf("the value is: SByte=%d\n", val);
                query.bindValue(":value",val);
                query.bindValue(":datatype","SByte");
                query.bindValue(":nodeid",nodeid);
                query.exec();
              }
              else if(rResp.results[i].value.type == &UA_TYPES[UA_TYPES_BYTE])
              {
                int val = *(UA_Byte*)rResp.results[i].value.data;
                if(debug) printf("the value is: Byte=%d\n", val);
                query.bindValue(":value",val);
                query.bindValue(":datatype","Byte");
                query.bindValue(":nodeid",nodeid);
                query.exec();
              }
              else if(rResp.results[i].value.type == &UA_TYPES[UA_TYPES_INT16])
              {
                int val = *(UA_UInt16*)rResp.results[i].value.data;
                if(debug) printf("the value is: Int16=%d\n", val);
                query.bindValue(":value",val);
                query.bindValue(":datatype","Int16");
                query.bindValue(":nodeid",nodeid);
                query.exec();
              }
              else if(rResp.results[i].value.type == &UA_TYPES[UA_TYPES_UINT16])
              {
                unsigned int val = *(UA_UInt16*)rResp.results[i].value.data;
                if(debug) printf("the value is: UInt16=%d\n", val);
                query.bindValue(":value",val);
                query.bindValue(":datatype","UInt16");
                query.bindValue(":nodeid",nodeid);
                query.exec();
              }
              else if(rResp.results[i].value.type == &UA_TYPES[UA_TYPES_INT32])
              {
                int val = *(UA_Int32*)rResp.results[i].value.data;
                if(debug) printf("the value is: Int32=%d\n", val);
                query.bindValue(":value",val);
                query.bindValue(":datatype","Int32");
                query.bindValue(":nodeid",nodeid);
                query.exec();
              }
              else if(rResp.results[i].value.type == &UA_TYPES[UA_TYPES_UINT32])
              {
                unsigned int val = *(UA_UInt32*)rResp.results[i].value.data;
                if(debug) printf("the value is: UInt32=%d\n", val);
                query.bindValue(":value",val);
                query.bindValue(":datatype","UInt32");
                query.bindValue(":nodeid",nodeid);
                query.exec();
              }
              else if(rResp.results[i].value.type == &UA_TYPES[UA_TYPES_INT64])
              {
                long val = *(UA_Int64*)rResp.results[i].value.data;
                if(debug) printf("the value is: Int64=%ld\n", val);

                //long to QVariable
                long long val1=(long long) val;
                QVariant val2(val1);
                query.bindValue(":value",val2);
                query.bindValue(":datatype","Int64");
                query.bindValue(":nodeid",nodeid);
                query.exec();
              }
              else if(rResp.results[i].value.type == &UA_TYPES[UA_TYPES_UINT64])
              {
                unsigned long val = *(UA_UInt64*)rResp.results[i].value.data;
                if(debug) printf("the value is: UInt64=%ld\n", val);
                //unsigned long to QVvariable
                unsigned long long val1=(unsigned long long) val;
                QVariant val2(val1);
                query.bindValue(":value",val2);
                query.bindValue(":datatype","UInt64");
                query.bindValue(":nodeid",nodeid);
                query.exec();
              }
              else if(rResp.results[i].value.type == &UA_TYPES[UA_TYPES_FLOAT])
              {
                double val = *(UA_Double*)rResp.results[i].value.data;
                if(debug) printf("the value is: FLOAT=%f\n", (float) val);
                query.bindValue(":value",val);
                query.bindValue(":datatype","Float");
                query.bindValue(":nodeid",nodeid);
                query.exec();
              }
              else if(rResp.results[i].value.type == &UA_TYPES[UA_TYPES_DOUBLE])
              {
                double val = *(UA_Double*)rResp.results[i].value.data;
                if(debug) printf("the value is: DOUBLE=%f\n", (float) val);
                query.bindValue(":value",val);
                query.bindValue(":datatype","Double");
                query.bindValue(":nodeid",nodeid);
                query.exec();
              }
              else if(rResp.results[i].value.type == &UA_TYPES[UA_TYPES_STRING])
              {
                UA_Variant value = rResp.results[i].value;
                UA_String *ua_string = (UA_String*) value.data;
                //members:
                //  UA_Int32 length;
                //  UA_Byte *data;
                if(ua_string->length >= 0)
                {
                  char txt[ua_string->length + 1];
                  strncpy(txt,(const char *) ua_string->data,ua_string->length);
                  txt[ua_string->length] = '\0';
                  if(debug) printf("the value is: STRING=%s\n",txt);
                query.bindValue(":value",txt);
                query.bindValue(":datatype","String");
                query.bindValue(":nodeid",nodeid);
                query.exec();
                }
                else
                {
                  printf("Error: ua_string->length=%d\n", ua_string->length);
                }
              }
              else if(rResp.results[i].value.type == &UA_TYPES[UA_TYPES_DATETIME])
              {
                long val = *(UA_Int64*)rResp.results[i].value.data;
                if(debug) printf("the value is: DATETIME Int64=%ld\n", val);
                //long to QVariable
                long long val1=(long long) val;
                QVariant val2(val1);
                query.bindValue(":value",val2);
                query.bindValue(":datatype","DateTime");
                query.bindValue(":nodeid",nodeid);
                query.exec();
              }
              else if(rResp.results[i].value.type == &UA_TYPES[UA_TYPES_GUID])
              {
                printf("the value is: GUID (NOT IMPLEMENTED)\n");
              }
              else if(rResp.results[i].value.type == &UA_TYPES[UA_TYPES_BYTESTRING])
              {
                UA_Variant value = rResp.results[i].value;
                UA_String *ua_string = (UA_String*) value.data;
                //members:
                //  UA_Int32 length;
                //  UA_Byte *data;
                if(ua_string->length >= 0)
                {
                  char txt[ua_string->length + 1];
                  strncpy(txt,(const char *) ua_string->data,ua_string->length);
                  txt[ua_string->length] = '\0';
                  if(debug) printf("the value is: BYTESTRING=%s\n", txt);
                query.bindValue(":value",txt);
                query.bindValue(":datatype","ByteString");
                query.bindValue(":nodeid",nodeid);
                query.exec();
                }
                else
                {
                  printf("Error: ua_string->length=%d\n", ua_string->length);
                }
              }
              else if(rResp.results[i].value.type == &UA_TYPES[UA_TYPES_XMLELEMENT])
              {
                UA_Variant value = rResp.results[i].value;
                UA_String *ua_string = (UA_String*) value.data;
                //members:
                //  UA_Int32 length;
                //  UA_Byte *data;
                if(ua_string->length >= 0)
                {
                  char txt[ua_string->length + 1];
                  strncpy(txt,(const char *) ua_string->data,ua_string->length);
                  txt[ua_string->length] = '\0';
                  if(debug) printf("the value is: XMLELEMENT=%s\n", txt);
                query.bindValue(":value",txt);
                query.bindValue(":datatype","Xmlelement");
                query.bindValue(":nodeid",nodeid);
                query.exec();
                }
                else
                {
                  printf("Error: ua_string->length=%d\n", ua_string->length);
                }
              }
              else if(rResp.results[i].value.type == &UA_TYPES[UA_TYPES_NODEID])
              {
                printf("the value is: NODEID (NOT IMPLEMENTED)\n");
              }
              else if(rResp.results[i].value.type == &UA_TYPES[UA_TYPES_EXPANDEDNODEID])
              {
                printf("the value is: EXPANDEDNODEID (NOT IMPLEMENTED)\n");
              }
              else if(rResp.results[i].value.type == &UA_TYPES[UA_TYPES_STATUSCODE])
              {
                printf("the value is: STATUSCODE (NOT IMPLEMENTED)\n");
              }
              else if(rResp.results[i].value.type == &UA_TYPES[UA_TYPES_QUALIFIEDNAME])
              {
                printf("the value is: QUALIFIEDNAME (NOT IMPLEMENTED)\n");
              }
              else if(rResp.results[i].value.type == &UA_TYPES[UA_TYPES_LOCALIZEDTEXT])
              {
                printf("the value is: LOCALIZEDTEXT (NOT IMPLEMENTED)\n");
              }
              else if(rResp.results[i].value.type == &UA_TYPES[UA_TYPES_EXTENSIONOBJECT])
              {
                printf("the value is: EXTENSIONOBJECT (NOT IMPLEMENTED)\n");
              }
              else if(rResp.results[i].value.type == &UA_TYPES[UA_TYPES_DATAVALUE])
              {
                printf("the value is: DATAVALUE (NOT IMPLEMENTED)\n");
              }
              else if(rResp.results[i].value.type == &UA_TYPES[UA_TYPES_VARIANT])
              {
                printf("the value is: VARIANT (NOT IMPLEMENTED)\n");
              }
              else if(rResp.results[i].value.type == &UA_TYPES[UA_TYPES_DIAGNOSTICINFO])
              {
                printf("the value is: DIAGNOSTICINFO (NOT IMPLEMENTED)\n");
              }
              else
              {
                printf("the value is: NOT AN IMPLEMENTED TYPE\n");
              }
            }
            else
            {
              printf("the value is: NOT GOOD index=%d\n", i);
            }
        }
    }
    sleep(50000);

   }
}
int main(int argc, char *argv[])
{
    if(!ConnectToDB()){
        if(debug)
            qDebug()<<"连接失败";
    }
    if(!ReadFromDB()){
        if(debug)
            qDebug()<<"无法继续";
    }
   ReadFromServer();
//todo:disconnectfromdb
    QCoreApplication a(argc, argv);

    return a.exec();
}

