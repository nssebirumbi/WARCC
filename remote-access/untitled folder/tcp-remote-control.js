//'use strict';
const HOST = '0.0.0.0';
const PORT = 10028;
const fs = require('fs');

// Load the TCP Library
net = require('net');
// Keep track of the chat clients
var clients = [];
var k, cp=0, ii; // cp is a flag to tell when we are looping through commands paert of data.
var commands = "";

// Start a TCP Server
net.createServer(function (socket) {

	  // Identify this client
	  socket.name = socket.remoteAddress + ":" + socket.remotePort;
	  
	  /*add to this socket client a new attribute to be used for 
	    identifying the weather station where this client is communicating from as this is
	    needed in order to direct websocket clients' requests. 
		Initially set as 'anystation' string, but later this client has to submit his identity, 
		ie station name such as 'Entebbe'.  */
	  socket.identity = "anystation"; 

	  // Put this new client in the list
	  clients.push(socket);

	  // Send a nice welcome message and announce
	  socket.write("Welcome " + socket.name); 
	  console.log("\n---------------------New client:----------------\n"+ socket.name + " joined the chat\n");

	  // Handle incoming messages from clients.
	  socket.on('data', function(data){
			//when this client sends his identity, update his 'socket.identity' attribute with the received station name.
			var strdata = data.toString();
			if(strdata.startsWith("identity ")){ 
				socket.identity = strdata.split(" ")[1]; //the station name. message should be like: "identity entebbe-station"
				console.log("socket.identity = "+socket.identity);
			}
			//From now on, if any client wants to communicate to this client specifically, will specify this station name and 
			//then his message will be sent to only the client from this weather station
			if(strdata.startsWith("To ")){  //EXPECTED FORMAT: "To station-clientx data-to-send-to-the-client"
				var targetClient =  strdata.split(" ")[1];
				var darr = strdata.split(" ");
				let msgdata = ''; 
				for (var i in darr){
				    if(i>1){
				    msgdata += darr[i]+' ';
				    }
				}
				var reqData =  msgdata;
				console.log("targetClient= "+targetClient+", reqData = "+reqData);
				ForwardMsg(reqData, targetClient);  
			}
			
			if(strdata.startsWith("ToAll ")){  //EXPECTED FORMAT: "ToAll client1,client2,cl3 the-data-to-send-to-them" or "ToAll all the-data-to-send-to-them"
				var targetClients =  strdata.split(" ")[1];
				var darr = strdata.split(" ");
				let msgdata = ''; 
				for (var i in darr){
				    if(i>1){
				    msgdata += darr[i]+' ';
				    }
				}
				var reqData =  msgdata;
				broadcast(reqData,targetClients,socket);
				console.log("\n"); 
			}
			
			if(strdata.startsWith("login ")){  //EXPECTED FORMAT: "login username:user password:pass"
				var userData =  strdata.split(" ")[1]; //username:Dan
				var username = userData.split(":")[1];
				var userPass =  strdata.split(" ")[2]; //password:DanP1234
				var password = userPass.split(":")[1];
				//Read json file and fetch username and password
				
                let stringdata = fs.readFileSync('./user.json');
                let users = JSON.parse(stringdata);
                
                if((users[username] && users[username].username == username) && (users[username].password == password)) {
                    socket.write("login=ok");
                } else {
                    socket.write("login=failed");
                } 
			}
			
			if(strdata.startsWith("register ")){  //EXPECTED FORMAT: "register username:user password:pass"
				var userData =  strdata.split(" ")[1]; //username:user
				var username = userData.split(":")[1];
				var userPass =  strdata.split(" ")[2]; //password:pass
				var password = userPass.split(":")[1];
				 
                let stringdata = fs.readFileSync('./user.json');
                let jsondata = JSON.parse(stringdata);			
                				
                let newuserinfo = {
                    username: username,
                    password: password,
                    address:""
                }
                
                jsondata[username] = newuserinfo; //this will push the new user to the json file
                 
                let userinfo = JSON.stringify(jsondata);
                fs.writeFile('./user.json', userinfo, (err) => {
                    if (!err) {
                        console.log('File note written to');
                    }
                });
                //sucess message
			}
	  });

	  // Remove the client from the list when it leaves
	  socket.on('end', function () {
		clients.splice(clients.indexOf(socket), 1);
		console.log(socket.name + " left the chat.\n");
	  });
	  // setInterval(()=>{broadcast("pushed data "," ");},3000);
	  
	  // Send a reqData to all clients
	  function broadcast(reqData, targetClients, sender) {
		if(targetClients === "all"){
			clients.forEach(function (client) {  
			   if(client.identity !== sender.identity){ //don't send to the sender
					client.write(reqData);
			   }
			});
		}else if(targetClients.includes(",")){
			let targetClientsArr = targetClients.split(",");
			targetClientsArr.forEach(function (clientIdentity) {  
			   clients.forEach(function (client) {  
				   if(client.identity === clientIdentity){
						client.write(reqData); 
				   }
				});
			});
		}
		// Log it to the server output too
		process.stdout.write(reqData)
	  }
	  
	  function ForwardMsg(reqData,targetClient) {
		clients.forEach(function (client) { 
		  if(client.identity === targetClient){
			client.write(reqData);
		  }
		});
		// Log it to the server output too
		process.stdout.write(reqData)
	  }
	   
}).listen(PORT);

// Put a friendly message on the terminal of the server.
console.log("TCP server running at port 10028\n");







