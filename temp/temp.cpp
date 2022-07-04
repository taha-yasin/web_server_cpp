#include "crow_all.h"
#include <unordered_set>
#include <mutex>
using namespace std;
using namespace crow;

void sendFile(response &res, string filename, string contentType)
{
    ifstream in("../public/" + filename, ifstream::in);
    if (in)
    {
        ostringstream contents;
        contents << in.rdbuf();
        in.close();
        res.set_header("Content-Type", contentType);
        res.write(contents.str());
    }
    else
    {
        res.code = 404;
        res.write("Abhi bhi file nhii mil rha hai");
    }
    res.end();
}

void sendHtml(response &res, string filename)
{
    sendFile(res, filename + ".html", "text/html");
}

void sendImage(response &res, string filename)
{
    sendFile(res, "images/" + filename, "image/jpeg");
}

void sendScript(response &res, string filename)
{
    sendFile(res, "scripts/" + filename, "text/javascript");
}

void sendStyle(response &res, string filename)
{
    sendFile(res, "styles/" + filename, "text/css");
}

int main(int argc, char *argv[])
{
    std::mutex mtx;
    std::unordered_set<crow::websocket::connection *> users;
    crow::SimpleApp app;
    //set_base(".");

    CROW_ROUTE(app, "/ws")
        .websocket()
        .onopen([&](crow::websocket::connection &conn)
                {
                    std::lock_guard<std::mutex> _(mtx);
                    users.insert(&conn);
                })

        .onclose([&](crow::websocket::connection &conn, const string &reason)
                 {
                     std::lock_guard<std::mutex> _(mtx);
                     users.erase(&conn);
                 })

        .onmessage([&](crow::websocket::connection & /*conn*/, const string &data, bool is_binary)
                   {
                       std::lock_guard<std::mutex> _(mtx);
                       for (auto user : users)
                       {
                           if (is_binary)
                           {
                               user->send_binary(data);
                           }
                           else
                           {
                               user->send_text(data);
                           }
                       }
                   });

    CROW_ROUTE(app, "/chat")
    ([](const request &req, response &res)
     { sendHtml(res, "chat"); });

    CROW_ROUTE(app, "/styles/<string>")
    ([](const request &req, response &res, string filename)
     { sendStyle(res, filename); });

    CROW_ROUTE(app, "/scripts/<string>")
    ([](const request &req, response &res, string filename)
     { sendScript(res, filename); });

    CROW_ROUTE(app, "/images/<string>")
    ([](const request &req, response &res, string filename)
     { sendImage(res, filename); });

    CROW_ROUTE(app, "/")
    ([](const request &req, response &res)
     { sendHtml(res, "index"); });

    char *port = getenv("PORT");
    uint16_t iPort = static_cast<uint16_t>(port != NULL ? stoi(port) : 18080);
    cout << "PORT = " << iPort << "\n";
    app.port(iPort).multithreaded().run();
}