description('Tests for .valueAsDate with &lt;input type=datetimelocal>.');

var input = document.createElement('input');
input.type = 'datetime-local';

function valueAsDateFor(stringValue) {
    input.value = stringValue;
    return input.valueAsDate;
}

shouldBe('valueAsDateFor("")', 'null');
shouldBe('valueAsDateFor("1969-12-31T12:34:56.789")', 'null');
shouldBe('valueAsDateFor("1970-01-01T00:00:00.000")', 'null');
shouldBe('valueAsDateFor("2009-12-22T11:32:11")', 'null');

var successfullyParsed = true;
