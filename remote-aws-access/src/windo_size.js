    import React, { useLayoutEffect, useState } from 'react';

    export function useWindowSize() {
      const [size, setSize] = useState([0, 0]);
      
      useLayoutEffect(() => {
            function updateSize() { setSize([window.innerWidth, window.innerHeight]); }
            window.addEventListener('resize', updateSize);
            updateSize();
            return () => window.removeEventListener('resize', updateSize);
      }, []);
      
      return size;
    }


    export function ShowWindowDimensions(props) {
          const [width, height] = useWindowSize();
          return <span>Window size: {width} x {height},  props: {props}</span>;
    } 
    
    export function RetnWindowDimensions(props) {
          let objj = {wd:"120",ht:"34",extra:props}; 
          const [width, height] = useWindowSize();
          objj.wd = width; 
          objj.ht = height;
          return objj;
    }
