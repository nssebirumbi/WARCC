import { useState } from 'react';
import { singletonHook } from './react-singleton-hook/src/index';

const initShouldConnect = true;  
const initShouldIntroduceSelf = true;  
const initCurrentMsg = "connect wimea.mak.ac.ug:10028";  
//const initActiveTask = "reportMask"; 
const initActiveTask = "reportInterval";
const initActiveNode = "";
const initLogin = {password:"admin1234",username:"admin"};


let globalShouldConnect = () => { throw new Error('you must useShouldConnect before setting ShouldConnect'); }; 
let globalShouldIntroduceSelf = () => { throw new Error('you must useShouldIntroduceSelf before setting ShouldIntroduceSelf'); };
let globalCurrentMsg = () => { throw new Error('you must useCurrentMsg before setting CurrentMsg'); }; 
let globalActiveTask = () => { throw new Error('you must useActiveTask before setting ActiveTask'); };
let globalActiveNode = () => { throw new Error('you must useActiveNode before setting ActiveNode'); }; 
let globalLogin = () => { throw new Error('you must useLogin before setting Login'); }; 

//--------
export const useShouldConnect = singletonHook(initShouldConnect, () => {
  const [conn, setCon] = useState(initShouldConnect);
  globalShouldConnect = setCon;
  return conn;
});  
export const useShouldIntroduceSelf = singletonHook(initShouldIntroduceSelf, () => {
  const [intro, setIntro] = useState(initShouldIntroduceSelf);
  globalShouldIntroduceSelf = setIntro;
  return intro;
});
export const useCurrentMsg = singletonHook(initCurrentMsg, () => {
  const [msgg, setMsg] = useState(initCurrentMsg);
  globalCurrentMsg = setMsg;
  return msgg;
}); 
export const useActiveTask = singletonHook(initActiveTask, () => {
  const [task, setTask] = useState(initActiveTask);
  globalActiveTask = setTask;
  return task;
}); 

export const useActiveNode = singletonHook(initActiveNode, () => {
  const [node, setNode] = useState(initActiveNode);
  globalActiveNode = setNode;
  return node;
}); 

export const useLogin = singletonHook(initLogin, () => {
  const [logn, setLogn] = useState(initLogin);
  globalLogin = setLogn;
  return logn;
});
//--------

export const setShouldConnect = cn => globalShouldConnect(cn); 
export const setShouldIntroduceSelf = self => globalShouldIntroduceSelf(self);
export const setCurrentMsg = msg => globalCurrentMsg(msg); 
export const setActiveTask = task => globalActiveTask(task);
export const setActiveNode = node => globalActiveNode(node); 
export const setLogin = logn => globalLogin(logn); 

//







