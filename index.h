const char MAIN_page[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>

<script>
setInterval(function() {
getData();
}, 1000);
function getData() {
var xhttp = new XMLHttpRequest();
xhttp.onreadystatechange = function() {
  if (this.readyState == 4 && this.status == 200) {
    document.getElementById("focuserPosition").innerHTML =
    this.responseText;
  }
};
xhttp.open('GET', 'getFocuserPosition', true);
xhttp.send();
}
</script>
<body>
<div id="page">
  <form action=login method=POST >
    <input type='submit' name='action' value='Init' ><br><br>
    <input type='radio' name='speed' value='slow'>Slow
    <input type='radio' name='speed' value='fast'>Fast<br>
    <input type='radio' name='direction' value='CW'>CW
    <input type='radio' name='direction' value='CCW'>CCW<br>
    <input type='submit' name='action' value='start'>
    <input type='submit' name='action' value='stop'><br><br>
    Location:<input type='input' name='location' value=''>
    <input type='submit' name='action' value='goto'><br>
  </form>
</div>
<div>Location:<span id="focuserPosition">0</span>
</div>

</body>
</html>
)=====";
