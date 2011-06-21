description("Ensure that globalAlpha is applied correctly to text.");
var canvas = document.createElement('canvas');
var ctx = canvas.getContext('2d');
ctx.fillStyle = "black";
ctx.globalAlpha = 0.5;
ctx.font = "50px Arial";
ctx.fillText("A",50,50);
var imageData = ctx.getImageData(63, 22, 1, 1);
var imgdata = imageData.data;
shouldBeTrue("imgdata[3] == 127 || imgdata[3] == 128");
var successfullyParsed = true;
