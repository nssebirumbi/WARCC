package main

import (
	"flag"
	//"fmt"
	//"bufio"
	"strings"
	//"encoding/json"
	"html/template"
	"log"
	"net"
	"net/http" 
	"github.com/gorilla/websocket"
)

var addr = flag.String("addr", "0.0.0.0:10026", "websocket-tcp bridge service address")

var upgrader = websocket.Upgrader{} // use default options 

type Wc struct {
	Websocket *websocket.Conn
}


func (wscon *Wc) bridge(tcpcon net.Conn, mt int) {
	resBuf := make([]byte, 4096)
	defer wscon.Websocket.Close()
	defer tcpcon.Close()  
	for { 	
		length, err := tcpcon.Read(resBuf)  //read from tcpcon  
		if err != nil {
		    log.Println("readtcp error:", err) 
		} 
		if length > 0 {
			log.Println("RECEIVED from tcp: " + string(resBuf))
			err = wscon.Websocket.WriteMessage(mt, resBuf) //responding to the web client.
			if err != nil {
				log.Println("response Error:", err) 
				break
			}
		} 
	}
}


func createNewBridge(w http.ResponseWriter, r *http.Request) {
	upgrader.CheckOrigin = func(r *http.Request) bool { return true }
	webclient, _ := upgrader.Upgrade(w, r, nil)   
	connectedToTcp := false 
	var tcpcon net.Conn
	
	for {
		log.Println("Waiting for data from websocket client ...")
		datatype, dataFromClient, err := webclient.ReadMessage() 
		if err != nil {
			log.Println("!! Error:", err) 
		} 
		log.Println("Received data-: ",dataFromClient)
		log.Println("datatype: ",datatype," Data in string-: ",string(dataFromClient)) 
		msgtokens := strings.Fields(string(dataFromClient)) //breaking the received string message
		//TODO: do the above only if the message from client is a string and if not, skip above operation. 
		if(msgtokens[0]=="connect"){
		   tcpcon, err = net.Dial("tcp", msgtokens[1]) 
		   if err!=nil {
		   	 log.Println("Error while connecting to TCP-: ",err)
		   }else{
				connectedToTcp = true
		   	 /********************************************************/
		   	 wc := &Wc{Websocket:webclient}      /********************/ 
		     go wc.bridge(tcpcon, datatype)      /*******Bridge*******/
		     /********************************************************/
			}
		}
		if(connectedToTcp){
			if(msgtokens[0]=="disconnect"){  
			   connectedToTcp = false
			}else{
				if(msgtokens[0]=="identity" || msgtokens[0]=="mac" || msgtokens[0]=="To" || msgtokens[0]=="ToAll" || msgtokens[0]=="register" || msgtokens[0]=="ffff"){
					log.Printf("Forwarding to tcp socket: %s", dataFromClient)  
					_, err = tcpcon.Write(dataFromClient)  /* submits the data from client to TCP end point */
					if err != nil {
						log.Println("Failed to forward:", err) 
					}
				}
		    } 
		}  
	}
}


func home(w http.ResponseWriter, r *http.Request) {
	homeTemplate.Execute(w, "ws://"+r.Host+"/echo")
}
  

func main() {
	flag.Parse()
	log.SetFlags(0)
	http.HandleFunc("/bridge", createNewBridge)
	http.HandleFunc("/", home)
	log.Println("Websocket-tcp bridge listening on port: 10026") 
	log.Fatal(http.ListenAndServe(*addr, nil)) 
}



 
var homeTemplate = template.Must(template.New("").Parse(`
<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8">
<script>  
window.addEventListener("load", function(evt) {
    var output = document.getElementById("output");
    var input = document.getElementById("input");
    var ws;
    var print = function(message) {
        var d = document.createElement("div");
        d.innerHTML = message;
        output.appendChild(d);
    };
    document.getElementById("open").onclick = function(evt) {
        if (ws) {
            return false;
        }
        ws = new WebSocket("{{.}}");
        ws.onopen = function(evt) {
            print("OPEN");
        }
        ws.onclose = function(evt) {
            print("CLOSE");
            ws = null;
        }
        ws.onmessage = function(evt) {
            print("RESPONSE: " + evt.data);
        }
        ws.onerror = function(evt) {
            print("ERROR: " + evt.data);
        }
        return false;
    };
    document.getElementById("send").onclick = function(evt) {
        if (!ws) {
            return false;
        }
        print("SEND: " + input.value);
        ws.send(input.value);
        return false;
    };
    document.getElementById("close").onclick = function(evt) {
        if (!ws) {
            return false;
        }
        ws.close();
        return false;
    };
});
</script>
</head>

<body>
<table>
<tr><td valign="top" width="50%">
<p>Click "Open" to create a connection to the server, 
"Send" to send a message to the server and "Close" to close the connection. 
You can change the message and send multiple times.
<p>
<form>
<button id="open">Open</button>
<button id="close">Close</button>
<p><input id="input" type="text" value="Hello world!">
<button id="send">Send</button>
</form>
</td><td valign="top" width="50%">
<div id="output"></div>
</td></tr></table>
</body>
</html>
`))
