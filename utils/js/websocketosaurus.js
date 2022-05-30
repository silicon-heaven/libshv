//document.getElementById("edUri").value = "wss://nirvana.elektroline.cz:3778";
document.getElementById("edUri").value = "wss://localhost:3778";
document.getElementById("edUser").value = "iot";
document.getElementById("edPassword").value = "lub42DUB";

document.getElementById("edShvPath").value = ".broker/app";
document.getElementById("edMethod").value = "echo";
document.getElementById("edParams").value = "42";

let txtLog = document.getElementById("txtLog");
function debug(...args)
{
    let line = "";
    for (let i=0; i < args.length; i++) {
        if(i > 0)
            line += " "
        line += args[i];
    }
    txtLog.value += line + "\n";
    txtLog.scrollTop = txtLog.scrollHeight;
}

function sendMessage()
{
    if ( websocket && websocket.readyState == 1 ) {
        let shv_path = document.getElementById("edShvPath").value;
        let method = document.getElementById("edMethod").value;
        let params = document.getElementById("edParams").value;

        callRpcMethod(shv_path, method, params);
    }
}

let websocket = null;
let hello_phase = false;
let login_phase = false;
let broker_connected = false;
let request_id = 1
let isUseCpon = false

function initWebSocket(use_chainpack)
{
    let wsUri = document.getElementById("edUri").value
    let user = document.getElementById("edUser").value
    let password = document.getElementById("edPassword").value


    isUseCpon = !use_chainpack
    let txtLog = document.getElementById("txtLog");
    txtLog.value = "Connection to shvbroker using " + (isUseCpon? "Cpon": "ChainPack") + " serialization.";
    request_id = 1;
    try {
        checkSocket();
        if ( websocket && websocket.readyState == 1 )
            websocket.close();
        checkSocket();
        websocket = new WebSocket( wsUri );
        websocket.binaryType = "arraybuffer";
        checkSocket();

        websocket.onopen = function (evt) {
            checkSocket();
            debug("CONNECTED");
            callRpcMethod(null, "hello")
            //sendCpon('<1:1,8:' + request_id++ + ',10:"hello">i{}')
            hello_phase = true;
        };
        websocket.onclose = function (evt) {
            checkSocket();
            debug("DISCONNECTED");
        };
        websocket.onmessage = function (evt) {
            //console.log( "Message received :", evt.data );
            let rpc_val = datatoRpcValue(evt.data);
            debug("message received: " + rpc_val );
            let rpc_msg = new RpcMessage(rpc_val);
            if(hello_phase) {
                // <1:1,8:1>i{2:{"nonce":"1805368699"}}
                let rqid = rpc_msg.requestId().toInt();
                if(rqid == 1) {
                    // <T:RpcMessage,id:2,method:"login">i{params:{"login":{"password":"lub42DUB","type":"PLAIN","user":"iot"},"options":{"device":{"mountPoint":"test/agent1"},"idleWatchDogTimeOut":0}}}
                    let params = '{"login":{"password":"' + password + '","type":"PLAIN","user":"' + user + '"},"options":{"device":{"mountPoint":"test/websocketosaurus"},"idleWatchDogTimeOut": 60}}'
                    callRpcMethod(null, "login", params)
                    hello_phase = false;
                    login_phase = true;
                }
            }
            else if(login_phase) {
                // <T:RpcMessage,id:2>i{result:{"clientId":2}}
                let rqid = rpc_msg.requestId().toInt();
                if(rqid == 2) {
                    login_phase = false;
                    broker_connected = true;
                    debug('SUCCESS: connected to shv broker');
                }
            }
            else if(broker_connected) {
                if(rpc_msg.isRequest()) {
                    let method = rpc_msg.method().asString();
                    let resp = new RpcMessage();
                    if(method == "dir") {
                        resp.setResult(["ls", "dir", "appName"]);
                    }
                    else if(method == "ls") {
                        resp.setResult([]);
                    }
                    else if(method == "appName") {
                        resp.setResult("websocketosaurus");
                    }
                    else {
                        debug('ERROR: ' + "Method: " + method + " is not defined.");
                        resp.setError("Method: " + method + " is not defined.");
                    }
                    resp.setRequestId(rpc_msg.requestId());
                    resp.setCallerIds(rpc_msg.callerIds());
                    sendRpcMessage(resp);
                }
                else {
                    let txtResult = document.getElementById("txtResult");
                    // result == 2
                    // error == 3
                    let err = rpc_msg.error();
                    if(err) {
                        txtResult.value = err.toString()
                        txtResult.style.background = "salmon"
                    }
                    else {
                        let result = rpc_msg.result();
                        txtResult.value = result.toString()
                        txtResult.style.background = ""
                    }
                }
            }
        };
        websocket.onerror = function (evt) {
            debug('ERROR: ' + evt.data);
        };
        websocket.onclose = function (evt) {
            debug('socket close code: ' + evt.code);
        };
    } catch (exception) {
        debug('EXCEPTION: ' + exception);
    }
}

function stopWebSocket() {
    if (websocket)
        websocket.close();
}

function checkSocket()
{
    let lblState = document.getElementById("lblState");
    if (websocket != null) {
        let stateStr;
        switch (websocket.readyState) {
            case 0: {
                stateStr = "CONNECTING";
                break;
            }
            case 1: {
                stateStr = "OPEN";
                break;
            }
            case 2: {
                stateStr = "CLOSING";
                break;
            }
            case 3: {
                stateStr = "CLOSED";
                break;
            }
            default: {
                stateStr = "UNKNOW";
                break;
            }
        }
        debug("WebSocket state = " + websocket.readyState + " ( " + stateStr + " )");
        lblState.innerHTML = stateStr
    } else {
        debug("WebSocket is null");
        lblState.innerHTML = "no-socket"
    }
}

function callRpcMethod(shv_path, method, params)
{
    let rq = new RpcMessage();
    rq.setRequestId(request_id++);
    if(shv_path)
        rq.setShvPath(shv_path);
    rq.setMethod(method);
    if(params)
        rq.setParams(RpcValue.fromCpon(params));
    sendRpcMessage(rq);
}

function sendRpcMessage(rpc_msg)
{
    if(websocket && websocket.readyState == 1) {
        debug("sending rpc message:", rpc_msg.toString())
        if(isUseCpon)
            var msg_data = new Uint8Array(rpc_msg.toCpon());
        else
            var msg_data = new Uint8Array(rpc_msg.toChainPack());

        let wr = new ChainPackWriter();
        wr.writeUIntData(msg_data.length + 1)
        let dgram = new Uint8Array(wr.ctx.length + 1 + msg_data.length)
        let ix = 0
        for (let i = 0; i < wr.ctx.length; i++)
            dgram[ix++] = wr.ctx.data[i]

        if(isUseCpon)
            dgram[ix++] = Cpon.ProtocolType
        else
            dgram[ix++] = ChainPack.ProtocolType

        for (let i = 0; i < msg_data.length; i++)
            dgram[ix++] = msg_data[i]
        debug("sending " + dgram.length + " bytes of data")
        websocket.send(dgram.buffer)
    }
}

function datatoRpcValue(buff)
{
    let rd = new ChainPackReader(new UnpackContext(buff));
    let len = rd.readUIntData()
    let proto = rd.ctx.getByte();
    if(proto == Cpon.ProtocolType)
        rd = new CponReader(rd.ctx)
    else
        rd = new ChainPackReader(rd.ctx)
    let rpc_val = rd.read();
    //debug("msg buff len " + buff.byteLength + " offset: " + offset)
    //return String.fromCharCode.apply(null, new Uint8Array(buff, offset));
    return rpc_val;
}
