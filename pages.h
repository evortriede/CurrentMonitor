/*
 * Server Index Page
 */
 
const char* serverIndex = 
"<script src='https://ajax.googleapis.com/ajax/libs/jquery/3.2.1/jquery.min.js'></script>"
"<form method='POST' action='#' enctype='multipart/form-data' id='upload_form'>"
   "<input type='file' name='update'>"
        "<input type='submit' value='Update'>"
    "</form>"
 "<div id='prg'>progress: 0%</div>"
 "<script>"
  "$('form').submit(function(e){"
  "e.preventDefault();"
  "var form = $('#upload_form')[0];"
  "var data = new FormData(form);"
  " $.ajax({"
  "url: '/update',"
  "type: 'POST',"
  "data: data,"
  "contentType: false,"
  "processData:false,"
  "xhr: function() {"
  "var xhr = new window.XMLHttpRequest();"
  "xhr.upload.addEventListener('progress', function(evt) {"
  "if (evt.lengthComputable) {"
  "var per = evt.loaded / evt.total;"
  "$('#prg').html('progress: ' + Math.round(per*100) + '%');"
  "}"
  "}, false);"
  "return xhr;"
  "},"
  "success:function(d, s) {"
  "console.log('success!')" 
 "},"
 "error: function (a, b, c) {"
 "}"
 "});"
 "});"
 "</script>";

static const char rootFmt[] PROGMEM =R"(
<html>
  <head>
  </head>
  <body>
    <div style='font-size:250%%'>
      <form action="/set" method="GET">
        SSID to join <input type="text" name="ssid" style="font-size:40px" value="%s"></input><br>
        Password for SSID to join <input type="text" name="pass" style="font-size:40px" value="%s"></input><br>
        SSID for Captive Net <input type="text" name="captive_ssid" style="font-size:40px" value="%s"></input><br>
        Password for Captive Net SSID <input type="text" name="captive_pass" style="font-size:40px" value="%s"></input><br>
        Spreading Factor (7-12) <input type="text" name="sf" style="font-size:40px" value="%i"></input><br>
        Mosbus Server ip <input type="text" name="modbus_server" style="font-size:40px" value="%s"></input><br>
        Frequency to check tank level (in seconds) <input type="text" name="frequency" style="font-size:40px" value="%i"></input><br><br>
        <input type="submit"></input>
      </form>
      <br><a href="/reboot">Reboot</a>
    </div>
  </body>
</html>
)";
