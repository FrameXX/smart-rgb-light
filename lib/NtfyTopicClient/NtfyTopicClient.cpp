#include <NtfyTopicClient.h>

NtfyTopicClient::NtfyTopicClient(WiFiConnection &wifiConnection, String topic, MessageCallback messageCallback) : wifiConnection(wifiConnection), topic(topic), wifiClient(), websocketClient(), serverURL("wss://ntfy.sh/" + topic + "/ws"), messageCallback(messageCallback)
{
  this->setupWifiClient();
  this->setupWebsocketClient();
  this->connect();
}

void NtfyTopicClient::setupWifiClient()
{
  this->wifiClient.setInsecure();
}

void NtfyTopicClient::setupWebsocketClient()
{
  this->websocketClient.setInsecure();
  this->websocketClient.onMessage([this](const websockets::WebsocketsMessage &message)
                                  { this->onNewMessage(message); });
}

void NtfyTopicClient::connect()
{
  report("trying to connect to channel");
  if (!this->wifiConnection.getConnected())
    return;
  this->connected = this->websocketClient.connect(this->serverURL);
}

void NtfyTopicClient::disconnect()
{
  report("disconnecting from channel");
  this->websocketClient.close();
  this->connected = false;
}

void NtfyTopicClient::onNewMessage(websockets::WebsocketsMessage message)
{
  JsonDocument document;
  deserializeJson(document, message.data());
  if (document["event"] != "message")
    return;
  this->messageCallback(document["message"]);
}

void NtfyTopicClient::postMessage(String message)
{
  if (!this->wifiClient.connect("ntfy.sh", 443))
    return;
  String request = "POST /" + this->topic + " HTTP/1.1\r\n" + "Host: " + "ntfy.sh" + "\r\n" + "Content-Type: text/plain\r\n" + "Content-Length: " + String(message.length()) + "\r\n" + "Connection: close\r\n" + "\r\n" + message + "\r\n";
  this->wifiClient.print(request);
  this->wifiClient.stop();
}

void NtfyTopicClient::sendMessage(String message)
{
  if (!this->wifiConnection.getConnected())
    return;
  this->dispatcher = true;
  this->disconnect();
  this->postMessage(message);
  this->connect();
  this->dispatcher = false;
}

void NtfyTopicClient::keepAlive()
{
  reportValue(this->connected, "channel connection state");
  if (!this->connected && !this->dispatcher && this->wifiConnection.getConnected())
    this->connect();
}

void NtfyTopicClient::pollMessages()
{
  this->websocketClient.poll();
}