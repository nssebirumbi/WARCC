import React, { useState, useCallback, useEffect, useRef, createRef } from 'react';
import useWebSocket, { ReadyState } from 'react-use-websocket';
import css3 from "./App.css";
import 'bootstrap/dist/css/bootstrap.min.css';
import loginIllustrationImg from "./Login.PNG";
import dashboardImg from "./images/African-local-football.jpg";
import updateFirmwareIcon from "./images/updatex.png";
import programANodeIcon from "./images/progr.png";
import syncGatewayIcon from "./images/synchronize.png";
import reportIntervalIcon from "./images/intervall.png";
import dateTimeIcon from "./images/date-time.png";
import OnOffIcon from "./images/power-btn.png"; 
import stationIcon1 from "./images/weather-station2x.png";
import stationIcon2 from "./images/weather-station3.png";
import stationIcon3 from "./images/weather-station2.png";
import stationIcon4 from "./images/weather-station3.png";
import stationIcon5 from "./images/weather-station3.png";
import stationIcon6 from "./images/weather-station2.png";
import wimeaLogoBlurred from "./images/wimea-logo-blurred.png";
import Header from "./components/header";
import {useShouldConnect, setShouldConnect} from './components/globalFlags';
import {useShouldIntroduceSelf, setShouldIntroduceSelf} from './components/globalFlags';
import {useCurrentMsg, setCurrentMsg} from './components/globalFlags';
import {useActiveTask, setActiveTask} from './components/globalFlags';
import {useActiveNode, setActiveNode} from './components/globalFlags';
import {useLogin, setLogin} from './components/globalFlags';
import {Button, Navbar, Nav, NavDropdown, Container,Row,Col} from 'react-bootstrap';
// import { Multiselect } from 'multiselect-react-dropdown';
import MultiSelect from "react-multi-select-component";
//const localIpUrl = require('local-ip-url');
import Image from 'react-bootstrap/Image'

const App = () => {
	const data = [
		{'label':'ADC_1','value':'ADC_1'},
		{'label':'ADC_2','value':'ADC_2'},
		{'label':'ADC_3','value':'ADC_3'},
		{'label':'ADC_4','value':'ADC_4'},
		{'label':'RH','value':'RH'},
		{'label':'P','value':'P'},
		{'label':'T','value':'T'},
		{'label':'T1','value':'T1'},
		{'label':'V_A1','value':'V_A1'},
		{'label':'V_A2','value':'V_A2'},
		{'label':'V_IN','value':'V_IN'},
		{'label':'T_MCU','value':'T_MCU'},
		{'label':'V_MCU','value':'V_MCU'},
		{'label':'INTR','value':'INTR'},
		{'label':'P0_LST','value':'P0_LST'},
		{'label':'P0_LST60','value':'P0_LST60'},
		{'label':'WDSPD','value':'WDSPD'},
		{'label':'MAXWSD','value':'MAXWSD'}
	];
	const [options]=useState(data);
	const [selected, setSelected] = useState([]);

	
	  let opts = useRef;
  let shouldConnect = useShouldConnect(); 
  let shouldIntroduceSelf = useShouldIntroduceSelf();
  let currentMsg = useCurrentMsg();
  let activeTask = useActiveTask();
  let activeNode = useActiveNode();
  let login = useLogin();
  let stringMsgRef = useRef(<></>); 
  let paramsInputRef = useRef(<></>);
  let switchIndicatorRef = useRef(<></>);
  
  let newIntervalRef = useRef(10);
  let newNodeNameRef = useRef(""); 
  const [socketUrl, setSocketUrl] = useState('ws://localhost:10026/bridge'); //API that will echo messages sent to it back to the client
  const [messageHistory, setMessageHistory] = useState([]);
  const [currentReportingInterval, setCurrentReportingInterval] = useState("");
  const [showFeedbackBox,setShowFeedbackBox] = useState(false);
  const [activeStation,setActiveStation] = useState("Makerere");
  const [stationSwitch,setStationSwitch] = useState({Makerere:true,Mubende:true,Entebbe:true,Lwengo:true,KiigeKamuli:true,Jinja:true,Ikulwe:true,KideraBuyende:true});
  
  //const [activeTask,setActiveTask] = useState("reportMask"); 
  const [activePage,setActivePage] = useState("Login");
  const [reportMaskParams,setReportMaskParams] = useState([]);
  const [reportMaskActiveAction,setReportMaskActiveAction] = useState("None"); //willbe set to 'Add param' or 'Remove param'
  const [sendMessage, lastMessage, readyState, getWebSocket] = useWebSocket(socketUrl);
 
  const handleClickChangeSocketUrl = useCallback(() => setSocketUrl("ws://localhost:10026/echo"), []);
  //const handleClickSendMessage = useCallback((msg)=>{sendMessage(msg)}, [currentMsg]);
  const handleClickSendDisconnect = useCallback(() => sendMessage("disconnect localhost:10026"),[]);
   
  useEffect(() => {
    if (lastMessage !== null) { 
      //getWebSocket returns the WebSocket wrapped in a Proxy. This is to restrict actions like 
      //mutating a shared websocket, overwriting handlers, etc
      const currentWebsocketUrl = getWebSocket().url;
      console.log('received a message from server: ', lastMessage.data.toString());
      let lastmsg = lastMessage?lastMessage.data.toString():"";
        if(lastMessage && lastmsg.includes("login=ok")){ //'login=ok'
            setShowFeedbackBox(true);
	        setActivePage("Dashboard");
        }else if(lastMessage && lastmsg.includes("login=failed")){
            //handle login failed error
            alert("!! Login failed");
        } else if(lastMessage && lastmsg.includes("E64=")){
			//handle login failed error
			setActiveNode(lastMessage.data);
			console.log('received a message from server: ', lastMessage.data.toString());
			// alert("!! Login failed");
			
        }
    //   setMessageHistory((prev) => prev.concat(lastMessage));
    }
  }, [lastMessage]);
  
  const connectionStatus = {
    [ReadyState.CONNECTING]: 'Connecting',
    [ReadyState.OPEN]: 'Open',
    [ReadyState.CLOSING]: 'Closing',
    [ReadyState.CLOSED]: 'Closed',
  }[readyState];
  
  
  if(readyState === ReadyState.CONNECTING) {
		console.log("ws Connecting ...");
	}else if(readyState === ReadyState.OPEN && shouldConnect){
		console.log("ws Connected");
		if (shouldConnect){
		    setShouldConnect(false);
		    sendMessage(currentMsg); //connecting to tcp
		}
		if (shouldIntroduceSelf){
		    setShouldIntroduceSelf(false);
		    sendMessage("identity browser"); 
		}
		
		console.log("tcp Connected");
		//let thisMachinesIP = localIpUrl() // => 192.168.31.69
		//let thisMachinesIP = localIpUrl('public') // => 192.168.31.69
		//let thisMachinesIP = localIpUrl('public', 'ipv4') // => 192.168.31.69
		//let thisMachinesIP = localIpUrl('public', 'ipv6') // => fe80::c434:2eff:fe06:f90
		//let thisMachinesIP = localIpUrl('private') // => 127.0.0.1
		//let thisMachinesIP = localIpUrl('private', 'ipv4') // => 127.0.0.1
		//let thisMachinesIP = localIpUrl('private', 'ipv6') // => fe80::1
		//sendMessage("identity browserOnIp"+thisMachinesIP); //connecting to tcp
	}else if(readyState === ReadyState.CLOSING){
		console.log("ws Disconnecting ... ");
		sendMessage("disconnect wimea.mak.ac.ug:10028"); //disconnecting from tcp
		console.log("tcp Disconnected");
	}else if(readyState === ReadyState.CLOSED){
		console.log("ws Disconnected "); 
		//setCurrentMsg("connect wimea.mak.ac.ug:10028");
		setShouldConnect(true);
	} 
	 
   const handleLogin = (e)=>{
		var username = document.getElementById("username").value;
		var password = document.getElementById("password").value;
		if(username!=="" && password!==""){
			sendMessage("login username:"+username+" password:"+password+""); 
			document.getElementById("username").value="";
			document.getElementById("password").value="";
			username="";
			password=""; 
			// setActivePage("Dashboard");
		}else{
			alert("Password and username are required ");
			setShowFeedbackBox(true); 
		}
	}

	const handleMACAddr=(node)=>{
		if(node!==""){
			sendMessage("mac station:"+activeStation+" node:"+node+"");

		}else{
			alert("Password and username are required ");
			setShowFeedbackBox(true); 
		}
	}
	
	const handleRegister = (e)=>{
		var username = document.getElementById("username").value;
		var password = document.getElementById("password").value;
		if(username!=="" && password!==""){
			sendMessage("register username:"+username+" password:"+password+""); 
			document.getElementById("username").value="";
			document.getElementById("password").value="";
			username="";
			password=""; 
		}
		setActivePage("Login");
	} 

	
	
	let triger = (params)=>{
	    setCurrentMsg(params.msg);
	    console.log("params.msg = "+params.msg); 
	    sendMessage(params.msg);
	};
	
	
	let requestForCurrentReportingInterval = () => {
	    sendMessage(stringMsgRef.current.value+'current');
	    console.log("requestForCurrentReportingInterval("+stringMsgRef.current.value+'current'+")");
	    let lastmsg = lastMessage?lastMessage.data.toString():"";
	    if(lastMessage && lastmsg.includes("current-reporting-interval=")){
	        let intervo = lastmsg.split("=")[1];
	        return intervo;
	    }else{
	        return 15;
	    }
	}; 
	let requestForCurrentReportMaskParams = () => {
	    sendMessage(stringMsgRef.current.value+'current');
	    console.log("requestForCurrentReportMaskParams("+stringMsgRef.current.value+'current'+")");
	    let lastmsg = lastMessage?lastMessage.data.toString():"";
	    if(lastMessage && lastmsg.includes("current-report-mask=")){
	        let maskparams = lastmsg.split("=")[1];
	        //update reportMaskParams
	        let tmpArr = maskparams.split(',');
	        setReportMaskParams([...tmpArr]);
	        return maskparams;
	    }else{
	        return "T, V_IN, V_A1 ";
	    }
	}; 
	
	let requestForCurrentNodeName = () => {
	    sendMessage(stringMsgRef.current.value+'current');
	    console.log("requestForCurrentReportMaskParams("+stringMsgRef.current.value+'current'+")");
	    let lastmsg = lastMessage?lastMessage.data.toString():"";
	    // if(lastMessage && lastmsg.includes("current-report-mask=")){
	    //     let maskparams = lastmsg.split("=")[1];
	    //     //update reportMaskParams
	    //     let tmpArr = maskparams.split(',');
	    //     setReportMaskParams([...tmpArr]);
	    //     return maskparams;
	    // }else{
	    //     return "T, V_IN, V_A1 ";
	    // }
	}; 

	let requestToSwitchStation = () => {
	    sendMessage(stringMsgRef.current.value+(stationSwitch[activeStation]?"#off":"#on"));
	    console.log("requestToSwitchStation("+stringMsgRef.current.value+(stationSwitch[activeStation]?"#off":"#on")+")"); 
	};
	
	let requestToChangeReportingInterval = () => {
	    sendMessage(stringMsgRef.current.value+newIntervalRef.current.value);
	    console.log("requestToChangeReportingInterval("+stringMsgRef.current.value+newIntervalRef.current.value+")"); 
	    setCurrentReportingInterval(newIntervalRef.current.value);
	};

	let requestToChangeNodeName = () => {
	    sendMessage(stringMsgRef.current.value+newNodeNameRef.current.value);
	    console.log("requestToChangeNodeName("+stringMsgRef.current.value+newNodeNameRef.current.value+")"); 
	    setCurrentReportingInterval(newNodeNameRef.current.value);
	};
	
	let requestToChangeReportMask = () => {
	    //prepare all params as a string
	    let updatedparams = '';
	    for(var i in reportMaskParams){
	        updatedparams += '-#'+reportMaskParams[i];
	    }
	    sendMessage(stringMsgRef.current.value+''+updatedparams);
	    console.log("requestToChangeReportMask("+stringMsgRef.current.value+'#'+updatedparams+")");  
	};
	let addReportMaskParam = () => { 
	    //  let newParam = paramsInputRef.current.value;
	    //  let tempArr = reportMaskParams; 
	    //  if(!tempArr.includes(newParam)){
	    //     tempArr.push(newParam); 
		//  }
		// let newParam = opts.current.getSelectedItems;
		let newP="";
		if (selected.length!=0) {
			for(var i in selected){
				if(selected.length===1){
				   newP+=selected[i].value;
				}else{
				   if ((selected.length-3)===i) {
					   newP+=selected[i].value;
				   }else{
					   newP+=selected[i].value;
					   newP+="#";
				   }
				}
				
			}
		   let tempArr = reportMaskParams; 
		   tempArr.push(newP); 
		   setReportMaskParams([...tempArr]);
		   
		}
		if (newP!="") {
			sendMessage(stringMsgRef.current.value+'+#'+newP);
	    	console.log("requestToChangeReportMask("+stringMsgRef.current.value+'+#'+newP+")");
		}
			// newP="";
		// let opt
	     
	};
	let removeReportMaskParam = () => {
		let newP="";
		if (selected.length!=0) {
			for(var i in selected){
				if(selected.length===1){
				   newP+=selected[i].value;
				}else{
				   if ((selected.length-1)===i) {
					   newP+=selected[i].value;
				   }else{
					   if (selected[i].value===" ") {
						   break;
					   }else{
						newP+=selected[i].value;
						newP+="#";
					   }
					   
				   }
				}
				
			}
		   let tempArr = reportMaskParams; 
		   tempArr.push(newP); 
		   setReportMaskParams([...tempArr]);
		   
		}
		if (newP!="") {
			
			sendMessage(stringMsgRef.current.value+'-#'+newP);
	    	console.log("requestToChangeReportMask("+stringMsgRef.current.value+'-#'+newP+")");
		}
	    //  let tempArr = reportMaskParams; 
	    //  tempArr.pop();
	    //  setReportMaskParams([...tempArr]);
	};
	
	let tabStatusActive = (ev)=>{
	    let taskLabel = ev.target.textContent; 
	    setReportMaskActiveAction(taskLabel);
	}
	  
  if(activePage==="Login" || activePage==="Register"){
	  return (
	    <div> 
	       
	      <div id="root-div">

				<Header socketStatus={connectionStatus}/>
				
				<div id="content-div">
					<div id="sidebar-left">
						<div id="usage-div">
							<h2>Usage:</h2>
							Start by signing in to go to the dashboard and use the control menu to: <br/>
							<ul>
								<li>Program a node </li>
								<li>Set date / time </li>
								<li>Change reporting interval </li>
								<li>Synchronize the gateway </li> 
								<li> Switch on / off a device </li>
							</ul>
							If you want to program a node, you should have the firmware already compiled into a .hex file which 
							you will be required to upload, or if the .hex file is already uploaded to the gateway, then you will be required 
							to send a command instructiong the gateway device on how to use that firmware file.
						</div>
					</div>
					<div id="sidebar-right">
						{activePage==="Register"?
						<>
						<div id="login-div">   
							<br/>
							<img src={loginIllustrationImg} width="25%" height="25%"  alt=""/> <br/> <br/>
							<input type="text"  placeholder=" User name" id="username"/> <br/> <br/>
							<input type="password" placeholder=" Password" id="password"/>  <br/> <br/> <br/>
							<input type="password" placeholder="Re-type Password" id="password"/>  <br/> <br/> <br/>
							<button id="login-btn" onClick={handleRegister}>Register</button>
							<br/> <br/>
						</div>
						<div style={{marginLeft:"14px",display:"none"}} onClick={()=>setActivePage("Login")}>Have an account already ? <span style={{color:"blue",cursor:"pointer"}}>Sign in</span></div>
						</>
						:
						<>
						<div id="login-div">   
							<br/>
							<img src={loginIllustrationImg} width="25%" height="25%"  alt=""/> <br/> <br/>
							<input type="text"  placeholder=" User name" id="username"/> <br/> <br/>
							<input type="password" placeholder=" Password" id="password"/>  <br/> <br/> <br/>
							<Button id="login-btn" onClick={handleLogin}>Login</Button>
							<br/> <br/>
						</div>
						<div style={{marginLeft:"14px",display:"none"}} onClick={()=>setActivePage("Register")}>Have no account ? <span style={{color:"blue",cursor:"pointer"}}>Register</span></div>
						</>
						}
					</div>
				</div>
				
			</div>
			<div id="feeback" style={{display:showFeedbackBox?"block":"none"}}> ... </div> 
	    </div>
	  );
	}else if(activePage==="Dashboard"){
		return (
	    <div>
	      <div id="root-div">
 
				<Header socketStatus={connectionStatus}/>
				
				<div id="dashboard-content-div" style={{width:"100%",position:"relative",float:"left",padding:"0",margin:"0"}}>
					<div id="dashboard-sidebar-left" style={{width:"20%",position:"relative",float:"left",margin:"0"}}>
						<div id="image-div" style={{width:"100%",position:"relative",float:"left"}}>
							<img src={dashboardImg} width="100%" alt=""/>
						</div>
						<div id="side-links-div"> 
							<div className={(activeStation==="Makerere")?"station-link-list active":"station-link-list"} onClick={()=>setActiveStation("Makerere")}> <div><img src={stationIcon1} width="20px" alt=""/></div><div>Makerere</div> </div>
							<div className={(activeStation==="Mubende")?"station-link-list active":"station-link-list"} onClick={()=>setActiveStation("Mubende")}> <div><img src={stationIcon2} width="20px" alt=""/></div><div>Mubende </div> </div>
							<div className={(activeStation==="Entebbe")?"station-link-list active":"station-link-list"} onClick={()=>setActiveStation("Entebbe")}> <div><img src={stationIcon3} width="20px" alt=""/></div><div>Entebbe </div> </div>
							<div className={(activeStation==="Lwengo")?"station-link-list active":"station-link-list"} onClick={()=>setActiveStation("Lwengo")}> <div><img src={stationIcon4} width="20px" alt=""/></div><div>Lwengo </div> </div>
							<div className={(activeStation==="Kiige-Kamuli")?"station-link-list active":"station-link-list"} onClick={()=>setActiveStation("Kiige-Kamuli")}> <div><img src={stationIcon5} width="20px" alt=""/></div><div>Kiige-Kamuli </div> </div>
							<div className={(activeStation==="Jinja")?"station-link-list active":"station-link-list"} onClick={()=>setActiveStation("Jinja")}> <div><img src={stationIcon6} width="20px" alt=""/></div><div>Jinja </div> </div>
							<div className={(activeStation==="Ikulwe")?"station-link-list active":"station-link-list"} onClick={()=>setActiveStation("Ikulwe")}> <div><img src={stationIcon1} width="20px" alt=""/></div><div>Ikulwe </div> </div>
							<div className={(activeStation==="Kidera-Buyende")?"station-link-list active":"station-link-list"} onClick={()=>setActiveStation("Kidera-Buyende")}> <div><img src={stationIcon4} width="20px" alt=""/></div><div>Kidera-Buyende </div> </div>
						</div>
					</div>
					
					<div id="control-area-parent" style={{width:"80%",position:"relative",float:"left",padding:"0",margin:"0"}}>
					<div style={{background:"rgb(144,238,144,0.5)",minHeight:"10px",float:"left",position:"relative", width:"100%",textAlign:"center"}}> 
						<p><b>Controlling {activeStation} weather station</b></p>
					</div>
					<div id="control-area" style={{width:"98%",position:"relative",float:"left", border:"2px solid lightgray",borderRadius:"6px",padding:"5px",minHeight:"400px",margin:"4px"}}>
						<div id="control-tools" style={{width:"100%",position:"relative",float:"left"}}>
							<Navbar bg="light" expand="lg" variant="light" mb="6">
								<Navbar.Toggle aria-controls="basic-navbar-nav" />
								<Navbar.Collapse id="basic-navbar-nav">
									<Nav className="mr-auto">
									<NavDropdown title="Report Interval" >
										<NavDropdown.Item onClick={ ()=>{handleMACAddr("2m"); setActiveTask("reportInterval"); } }>2m</NavDropdown.Item>
										<NavDropdown.Item onClick={ ()=>{handleMACAddr("10m"); setActiveTask("reportInterval"); } }>10m</NavDropdown.Item>
										<NavDropdown.Item onClick={ ()=>{handleMACAddr("gnd"); setActiveTask("reportInterval"); } }>GND</NavDropdown.Item>
									</NavDropdown>
									<NavDropdown title="Report Mask" >
										<NavDropdown.Item onClick={ ()=>{handleMACAddr("2m"); setActiveTask("reportMask");} }>2m</NavDropdown.Item>
										<NavDropdown.Item onClick={ ()=>{handleMACAddr("10m"); setActiveTask("reportMask"); } }>10m</NavDropdown.Item>
										<NavDropdown.Item onClick={ ()=>{handleMACAddr("gnd"); setActiveTask("reportMask"); } }>GND</NavDropdown.Item>
									</NavDropdown>
									<NavDropdown title="Reset Node" >
										<NavDropdown.Item onClick={ ()=>{handleMACAddr("2m"); setActiveTask("onOff"); } }>2m</NavDropdown.Item>
										<NavDropdown.Item onClick={ ()=>{handleMACAddr("10m"); setActiveTask("onOff"); } }>10m</NavDropdown.Item>
										<NavDropdown.Item onClick={ ()=>{handleMACAddr("gnd"); setActiveTask("onOff"); } }>GND</NavDropdown.Item>
									</NavDropdown>
									<NavDropdown title="Change Node Name" >
										<NavDropdown.Item onClick={ ()=>{handleMACAddr("2m"); setActiveTask("nodeName"); } }>2m</NavDropdown.Item>
										<NavDropdown.Item onClick={ ()=>{handleMACAddr("10m"); setActiveTask("nodeName"); } }>10m</NavDropdown.Item>
										<NavDropdown.Item onClick={ ()=>{handleMACAddr("gnd"); setActiveTask("nodeName"); } }>GND</NavDropdown.Item>
									</NavDropdown>
									</Nav>
								</Navbar.Collapse>
							</Navbar>
							 {/* <div style={{width:"100%",position:"relative",float:"left"}}>

								<table width="100%">
									<tbody>
										<tr> 
											<td>
												<div onClick={ ()=>setActiveTask("reportInterval") } ><img src={reportIntervalIcon} width="20px" alt="Report interval"/> Report interval </div> 
												<div width="100%">
													<select>
														<option>2m</option>
														<option>10m</option>
														<option>Gnd</option>
													</select>
												</div>
											</td>
											<td><div onClick={ ()=>{setActiveTask("reportMask"); } } ><img src={programANodeIcon} width="20px" alt="reportMask"/> Report Mask </div> </td>
											<td><div onClick={()=>setActiveTask("onOff")} ><img src={OnOffIcon} width="20px" alt="On / Off"/> On / Off </div> </td>
											<td><div onClick={ ()=>setActiveTask("synchronize") } ><img src={syncGatewayIcon} width="20px" alt="synchronize"/> Synchronize </div> </td>
										</tr>
									</tbody> 
								</table>
							</div> */}
							<div style={{width:"50%", position:"relative",float:"left"}}>
								<table width="100%">
									<tbody>
										<tr> 
											<td></td>
											
										</tr>
									</tbody>
								</table>
							</div> 
						</div>
						
							<input  ref={stringMsgRef} type="text" value={activeTask=="reportMask"?"To "+activeStation+" report#"+activeNode+"#re#"
							:activeTask=="nodeName"?"To "+activeStation+" report#"+activeNode+"#n#":activeTask=="reportInterval"?"To "+activeStation+" report#"+activeNode+"#ri#":activeTask=="onOff"?"To "+activeStation+" report#boot":null} style={{width:"80%", margin:"60px 0 0 0",display:"none"}} />
						    
							<button onClick={()=>sendMessage(stringMsgRef.current.value) } style={{postion:"absolute", bottom:"0", right:"2%",display:"none"}}>
						        {activeTask}
						    </button>  
							
						    {activeTask=="reportInterval"?
						    <div id="feeback" style={{border:"2px solid limegreen", padding:"4px", marginTop:"20px"}}>
						        <h4> Reporting  interval </h4>
						        <table width="70%">
						            <thead>
						                <tr> <td style={{fontSize:"22px"}}>Current interval (in seconds)::</td> <td colspan="2" > {requestForCurrentReportingInterval()}</td> </tr>
						            </thead>
						            <tbody>
						                <tr>
						                    <td style={{fontSize:"30px"}}></td>
						                    <td> <input ref={newIntervalRef} type="number" style={{width:"90%",padding:"4px",fontSize:"30px"}}/> </td> 
						                    <td> <Button onClick={requestToChangeReportingInterval}>Change interval now</Button></td> 
						                </tr>
						                <tr>
						                    <td colspan="3" id="changeIntervalFeedback">
						                        {
						                        (lastMessage && lastMessage.data.toString().includes("changed-reporting-interval"))?
						                        <><br/><hr/>{lastMessage.data.toString().split(':')[1]}</>:
						                        //<><br/><hr/>{lastMessage.data.toString()}</>
						                        <small><i><br/><hr/>changing reporting interval to</i> {currentReportingInterval} ... </small>
						                        }
						                    </td> 
						                </tr>
						            </tbody>
						        </table>
							</div>

							:activeTask=="reportMask"?
						    ///////////////////
						    <div id="feeback" style={{border:"2px solid limegreen", padding:"4px" }}> 
						        
						        <h4> Report Mask </h4>
								<div>Current report mask parameters:: {requestForCurrentReportMaskParams()}</div>
								{/* <Multiselect options={options} value={selected} onChange={setSelected} displayValue="mask" selectionLimit="8" /> */}
								{/* <pre>{JSON.stringify(selected)}</pre> */}
								<MultiSelect
									options={options}
									value={selected}
									onChange={setSelected}
									labelledBy={"Select"}
									style={{width:"50%" }}
									width={64}
								/>
								<div style={{alignItems:"right", padding:"4px" }}>
									<Button onClick={addReportMaskParam}>Add Mask</Button>{' '}
									<Button onClick={removeReportMaskParam}>Remove Mask</Button>
								</div>
								
								<div id="changeIntervalFeedback">
									{
										(lastMessage && lastMessage.data.toString().includes("changed-report-mask"))?
										<><br/><hr/>{lastMessage.data.toString().split(':')[1]}</>:
										//<><br/><hr/>{lastMessage.data.toString()}</>
										<small><i><br/><hr/>changing report mask to</i> {reportMaskParams} ... </small>
									}
								</div>
								
							</div>
							////////////////

							:activeTask=="nodeName"?
						    ///////////////////
						    <div id="feeback" style={{border:"2px solid limegreen", padding:"4px" }}> 
						        
						        <h4> Change Node Name </h4>
								<table width="70%">
						            <thead>
						                <tr> <td style={{fontSize:"22px"}}>Current name </td> <td colspan="2">:: {requestForCurrentNodeName()}</td> </tr>
						            </thead>
						            <tbody>
						                <tr>
						                    <td style={{fontSize:"30px"}}></td>
						                    <td> <input ref={newNodeNameRef} type="text" style={{width:"90%",padding:"4px",fontSize:"30px"}}/> </td> 
						                    <td> <Button onClick={requestToChangeNodeName}>Change Node Name</Button></td> 
						                </tr>
						                <tr>
						                    <td colspan="3" id="changeIntervalFeedback">
						                        {
						                        (lastMessage && lastMessage.data.toString().includes("changed-reporting-interval"))?
						                        <><br/><hr/>{lastMessage.data.toString().split(':')[1]}</>:
						                        //<><br/><hr/>{lastMessage.data.toString()}</>
						                        <small><i><br/><hr/>changing node name to</i> {currentReportingInterval} ... </small>
						                        }
						                    </td> 
						                </tr>
						            </tbody>
						        </table>
								
							</div>
							////////////////

							:activeTask=="onOff"?
						    <div id="feeback" style={{border:"2px solid limegreen", padding:"4px" }}> 
						        time Date
								<Container>
									<Row>
										<Col xs={6} md={4}>
										<Image src="holder.js/171x180" rounded />
										</Col>
										<Col xs={6} md={4}>
										<Image src="holder.js/171x180" roundedCircle />
										</Col>
										<Col xs={6} md={4}>
										<Image src="holder.js/171x180" thumbnail />
										</Col>
									</Row>
								</Container>
						    </div>:activeTask=="synchronize"?
						    <div id="feeback" style={{border:"2px solid limegreen", padding:"4px" }}> 
						        synchronize
						    </div>:null
						    }
						    
						</div> 
					</div>
				</div>
				
			</div>
			
	    </div>
	  );
	}else{
	    return (
	        <div>
	            Page switch has a problem
	        </div>
	    );
	}
  
}
 
export default App;
 
 
 
 
