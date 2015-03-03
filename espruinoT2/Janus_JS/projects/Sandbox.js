/*************************************************
Initializations for the T2 Environment through 
the use of the onInit() automatic call and modules
*************************************************/

/************
//Application Globals
************/


/************
//Application includes/modules
************/
//Invoke the hardware - REQUIRED
var myHW = require("T2HW").T2HW();


function CycleDown() {
  digitalWrite([LED1,LED2,LED3],0b100);
  setTimeout("digitalWrite([LED1,LED2,LED3],0b010);", 200);
  setTimeout("digitalWrite([LED1,LED2,LED3],0b001);", 400);
  setTimeout("digitalWrite([LED1,LED2,LED3],0);", 600);
}
function CycleUp() {
  digitalWrite([LED1,LED2,LED3],0b001);
  setTimeout("digitalWrite([LED1,LED2,LED3],0b010);", 200);
  setTimeout("digitalWrite([LED1,LED2,LED3],0b100);", 400);
  setTimeout("digitalWrite([LED1,LED2,LED3],0);", 600);
}

function onInit() {
  /*****************
  This function runs on powerup, you can put RunApp() or something
  here to run things automatically.
  *****************/

  setTimeout("CycleDown();", 0);
  setTimeout("CycleUp();", 800);
}






























