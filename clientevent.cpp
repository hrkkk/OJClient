#include "clientevent.h"

ClientEvent::ClientEvent(QObject *parent)
    : QObject{parent}
{

}

/*
 * 登录事件报文格式:
 * {
 *   "Event":"Login",
 *   "Account":"xxx",
 *   "Password":"xxx"
 * }
 */
QByteArray ClientEvent::loginEvent(QString account, QString password)
{
    // 提取用户输入的信息，封装为JSON格式，并转为二进制字节流
    QJsonObject json_obj;
    json_obj.insert("Event", "Login");
    json_obj.insert("Account", account);
    json_obj.insert("Password", password);

    QJsonDocument json_doc(json_obj);
    QByteArray data = json_doc.toJson();

    return data;
}

/*
 * 注册事件报文格式:
 * {
 *   "Event":"Register",
 *   "Account":"xxx",
 *   "Password":"xxx"
 * }
 */
QByteArray ClientEvent::registerEvent(QString account, QString password)
{
    // 提取用户输入的信息，封装为JSON格式，并转为二进制字节流
    QJsonObject json_obj;
    json_obj.insert("Event", "Register");
    json_obj.insert("Account", account);
    json_obj.insert("Password", password);

    QJsonDocument json_doc(json_obj);
    QByteArray data = json_doc.toJson();

    return data;
}

/*
 * 问题事件报文格式:
 * {
 *   "Event":"Problem",
 *   "Identity":2,
 *   "Method":"Add"
 *   "Category":0,
 *   "Content":{
 *      "Question":"xxx",
 *      "OptionA":"x",
 *      "OptionB":"x",
 *      "OptionC":"x",
 *      "OptionD":"x",
 *      "Socre":x,
 *      "Answer":"xxx",
 *      "Analysis":"xxx"}
 * }
 */
QByteArray ClientEvent::problemAddEvent(QString question, QString optionA, QString optionB,
                                        QString optionC, QString optionD, int socre, QString answer, QString analysis)
{
    // 提取用户输入的信息，封装为JSON格式，并转为二进制字节流
    QJsonObject json_obj;
    json_obj.insert("Event", "Problem");
    json_obj.insert("Identity", 2);
    json_obj.insert("Method", "Add");
    json_obj.insert("Category", 0);

    QJsonObject json_ques;
    json_ques.insert("Question", question);
    json_ques.insert("OptionA", optionA);
    json_ques.insert("OptionB", optionB);
    json_ques.insert("OptionC", optionC);
    json_ques.insert("OptionD", optionD);
    json_ques.insert("Socre", socre);
    json_ques.insert("Answer", answer);
    json_ques.insert("Analysis", analysis);

    json_obj.insert("Content", json_ques);

    QJsonDocument json_doc(json_obj);
    QByteArray data = json_doc.toJson();

    return data;
}

/*
 * 问题事件报文格式:
 * {
 *   "Event":"Problem",
 *   "Identity":2,
 *   "Method":"Add"
 *   "Category":1,
 *   "Content":{
 *      "Question":"xxx",
 *      "Blank":"x",
 *      "Socre":x,
 *      "Answer":"xxx",
 *      "Analysis":"xxx"}
 * }
 */
QByteArray ClientEvent::problemAddEvent(QString question, int blank, QString answer,
                                        int socre, QString analysis)
{
    // 提取用户输入的信息，封装为JSON格式，并转为二进制字节流
    QJsonObject json_obj;
    json_obj.insert("Event", "Problem");
    json_obj.insert("Identity", 2);
    json_obj.insert("Method", "Add");
    json_obj.insert("Category", 1);

    QJsonObject json_ques;
    json_ques.insert("Question", question);
    json_ques.insert("Blank", blank);
    json_ques.insert("Answer", answer);
    json_ques.insert("Socre", socre);
    json_ques.insert("Analysis", analysis);

    json_obj.insert("Content", json_ques);

    QJsonDocument json_doc(json_obj);
    QByteArray data = json_doc.toJson();

    return data;
}
