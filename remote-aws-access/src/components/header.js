import React from 'react'; 
import wimeaLogo from "../images/wimea-logo.png";

//==============================================================================
const Header = props => {
         let SystemStatusStyle = {background:"rgb(144,238,144,0.5)",border:"1px solid rgb(144,238,144,0.5)"};
         const styleSystemStatus = ()=>{
				switch(props.socketStatus){
					case "Connecting":
						SystemStatusStyle = {background:"blue",border:"3px solid yellow"};
						break;
					case "Open":
						SystemStatusStyle = {background:"skyblue",border:"3px solid white"};
						break;
					case "Closing":
						SystemStatusStyle = {background:"yellow",border:"3px solid yellow"};
						break;
					default:
						SystemStatusStyle = {background:"red",border:"3px solid yellow"}
				}
			};
			styleSystemStatus();
    return (
			<div id="header-div">
				<div  id="top-title-div"> 
					<small id="small1">+256 757 734717 SCIT,</small>   <small id="small2">7062 Kampala Uganda</small>
				</div>
				<div id="head">
					<div id="system-title-div">
						<small style={SystemStatusStyle} id="small0"></small> THE REMOTE AWS ACCESS SYSTEM
					</div>
					<div id="wimea-logo-div">
						<img src={wimeaLogo} alt=""/>
					</div>
					<div id="mak-logo-div"> 
						<img src="https://www.mak.ac.ug/sites/default/files/mak-logo-sm.png" alt=""/>
					</div>
				</div>
				<div >
					<br/>
					Controlling weather stations remotely over cellular network using technologies like websockets,
					RPC, TCP, React, Golang and more. 
				</div>
			</div>
		); 
}

export default Header;

