// built-in functions

foo = "foo bar stuff";
r = Math.random();

parsed = parseInt("42");

aStr = "ABCD";
aChar = aStr.charAt(0);
bChar = aStr.charAt(1);

obj1 = new Object();
obj1.food = "cake";
obj1.desert = "pie";

obj2 = obj1.clone();
obj2.food = "kittens";

var a = [
  foo.length===13, 
  foo.indexOf("bar")===4, 
  foo.substring(8,13)==="stuff",
  parsed===42,
  aChar.charCodeAt(0)===65,
  obj1.food==="cake",
  obj2.desert==="pie",
  aChar==="A",
  bChar==="B",
  aStr.charAt(-1)==="",
  aStr.charAt(1000)==="",
];

pass = 0;
a.forEach(function (n) { if (n) pass++; });
result = pass==a.length;
