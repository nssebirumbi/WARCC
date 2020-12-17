import React,{useState,useEffect} from 'react'; 
import axios from 'axios';  

export const useHttp = (url, requestObject, uniqueString) => { 
     console.log("uniqueString: "+uniqueString);
  const [requestHeaders,setRequestHeaders] = useState({"Content-Type":"application/json"});
  const [requestBody,setRequestBody] = useState(requestObject.Body);
  const [file,setFile] = useState(requestObject.File);   
  //----  
  const [responseState,setResponseState] = useState(null);
  const [responseErr,setResponseErr] = useState(null);  
  //---- 
  useEffect(() => { 
            if(url !== '/'){
            console.log("running in the useEffect");
            console.log(url);
            console.log(requestBody);
        setResponseState(null); 
        
        setRequestHeaders(requestObject.Headers);
        setRequestBody(requestObject.Body);
        setFile(requestObject.File);
        
        const submit = async () => {      
            let fData = new FormData();  
            fData.append('file',file);
            fData.append('body',requestBody);    
            const options = {
                            method: 'post',
                            url: url, 
                            data: fData, 
                            mode: 'no-cors', 
                            headers: requestHeaders 
                            };           
            axios(options).then((response) => {
                          console.log("response----------");
                          console.log(response);
                          setResponseState(response);  
                          return {loading:false,respdata:responseState,erro:null};
                      }).catch((error) => {
                          console.log("response error----------");
                          console.log(error);
                          setResponseErr(error);  
                          return {loading:false,respdata:null,erro:responseErr};
                      }); 
      } //end of submit
      
      submit();
      while(1){}
    }
   },[uniqueString]); 
}
 
export function MakeRequest(props) {
    const returnobject = useHttp(props.url, props.reqstbody,props.newRequestId);
    return returnobject
}




