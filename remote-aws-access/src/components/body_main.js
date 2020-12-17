import React,{useState, useRef} from 'react';  
import {useHttp} from '../shared-hooks/http'; 
import rslogo from '../rento_log.png'; 
import uslogo from '../images/user1.png'; 

const Header = () => { 
    const [url,setUrl] = useState("/");
    const [headers,setHeaders] = useState({"Content-Type":"multipart/mixed","Token":"a32145cd6c23e1200b"});
    const [reqbody,setReqBody] = useState(`{"title":"the coolest book ever", "isbn":"1587873","author":{"firstname":"ROgers","lastname":"Engineer","nationality":"Canadian"} }`);
    const [reqfile,setReqFile] = useState({});   
    const [reqfilename,setReqFilename] = useState("");
    const [domElementId,setDomElementId] = useState(""); 
    const fileRef = useRef(); 
    const linkRef = useRef();
    const [isLoading,responseState,responseErr] = useHttp(url, {Headers:headers, Body:reqbody,File:reqfile },domElementId);
     //----------------------------------------functions:----------------------- 
    const handleFileChange = (ChosenFile) => { 
        setReqFile(ChosenFile); 
        setReqFilename(ChosenFile.name);
        console.log('reqfilename: '+reqfilename+'<->'+ChosenFile.name);
        console.log('reqfile: '+reqfile); 
        setUrl("/api/books");
        setDomElementId(reqfilename);  
    }
     
    //-------------------------------------------------------------------------- 
    
    return (  
    <div> 
        <header id="header" className="theheader"> 
            <div id="stuck_container">
                <div className="container">
                    <div className="row">
                        <div className="grid_12">
                            <div className="brand put-left">
                                <h1>
                                    <a href="index.html">
                                        <img src={rslogo} alt="Logo" width="20%"/>
                                    </a>
                                </h1>
                            </div>
                            <nav className="nav put-right">
                                <ul className="sf-menu">  
                                    <li>test useRef<br/><input type="button" onClick={(e) => {linkRef.current.innerHTML=fileRef.current.value ; fileRef.current.click()} } value="Test"/> </li>
                                    <li ref={linkRef}>filename </li>
                                    <li value="some text">link3</li>
                                    <li><input type="file" ref={fileRef} id="file1" onChange={(e) => {handleFileChange(fileRef.current.value); setReqFilename(fileRef.current.value.name)} } /></li>
                                </ul>
                            </nav>
                        </div>
                    </div>
                </div>
            </div> 
        </header>   
 
        </div>
     );

}

export default Header;

