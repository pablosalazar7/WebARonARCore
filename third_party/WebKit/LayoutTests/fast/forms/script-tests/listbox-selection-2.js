description('&lt;select> selection test for scrolling.');

var parent = document.createElement('div');
parent.innerHTML = '<select id="sl1" multiple size=5>'
    + '<option>one</option>'
    + '<option>two</option>'
    + '<option>three</option>'
    + '<option>four</option>'
    + '<option>five</option>'
    + '<option>six</option>'
    + '<option>seven</option>'
    + '<option>eight</option>'
    + '<option>nine</option>'
    + '<option>ten</option>'
    + '<option>eleven</option>'
    + '<option>twelve</option>'
    + '<option>thirteen</option>'
    + '<option>fourteen</option>'
    + '<option>fifteen</option>'
    + '<option>sixteen</option>'
    + '<option>seventeen</option>'
    + '</select>'
    + '<select id="sl2" multiple style="height: 135px; border: 10px solid; padding: 5px;">'
    + '<option>one</option>'
    + '<option>two</option>'
    + '<option>three</option>'
    + '</select>';
document.body.appendChild(parent);

// Determine the item height.
var sl1 = document.getElementById('sl1');
var sl2 = document.getElementById('sl2');
var itemHeight = Math.floor(sl1.offsetHeight / sl1.size);
sl1.removeAttribute('size');
var height = itemHeight * 9 + 9;
sl1.setAttribute('style', 'height: ' + height + 'px; border: 10px solid; padding: 5px;');

function mouseDownOnSelect(selId, index)
{
    var sl = document.getElementById(selId);
    var borderPaddingTop = 15;
    var borderPaddingLeft = 15;
    var y = index * itemHeight + itemHeight / 3 - window.pageYOffset + borderPaddingTop;
    var event = document.createEvent("MouseEvent");
    event.initMouseEvent("mousedown", true, true, document.defaultView, 1, sl.offsetLeft +  borderPaddingLeft, sl.offsetTop + y, sl.offsetLeft + borderPaddingLeft, sl.offsetTop + y, false, false, false, false, 0, document);
    sl.dispatchEvent(event);
}

function selectionPattern(selectId)
{
    var select = document.getElementById(selectId);
    var result = "";
    for (var i = 0; i < select.options.length; i++)
        result += select.options[i].selected ? '1' : '0';
    return result;
}

mouseDownOnSelect("sl1", 0);
shouldBe('selectionPattern("sl1")', '"10000000000000000"');

mouseDownOnSelect("sl1", 1);
shouldBe('selectionPattern("sl1")', '"01000000000000000"');

mouseDownOnSelect("sl1", 6);
shouldBe('selectionPattern("sl1")', '"00000010000000000"');

mouseDownOnSelect("sl1", 7);
shouldBe('selectionPattern("sl1")', '"00000001000000000"');

mouseDownOnSelect("sl1", 8);
shouldBe('selectionPattern("sl1")', '"00000001000000000"');

mouseDownOnSelect("sl1", 0);
shouldBe('selectionPattern("sl1")', '"01000000000000000"');

for (i = 0; i < 9; i++)
    mouseDownOnSelect("sl1", 7);
shouldBe('selectionPattern("sl1")', '"00000000000000001"');

mouseDownOnSelect("sl2", 1);
shouldBe('selectionPattern("sl2")', '"010"');

mouseDownOnSelect("sl2", 3);
shouldBe('selectionPattern("sl2")', '"010"');

mouseDownOnSelect("sl2", 2);
shouldBe('selectionPattern("sl2")', '"001"');

var successfullyParsed = true;
