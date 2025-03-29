#include "pages.h"

const char* getIndexPage() { 
    return R"====(
<html> <!-- original .html file -->
<head>
    <title>Configure password</title>
</head>
<style>
    html {
     font-family: Arial;
     text-align: center;
    }
    #wholeContainer{
        padding-left: 10vw;
        padding-right: 10vw;
        padding-bottom: 10vh;
        height: 100vh;
        display: flex;
        flex-direction: column;
        justify-content: center;
    }
    form {
        display: flex;
        flex-direction: column;
        
    }
    input{
        margin-bottom: 1vh;
        height: 5vh;
        font-size: 3vh;
    }

    h1 {
        font-size: 3vh;
    }
  </style>
<body>
    <div id="wholeContainer">   
        <h1>Hi-horizon Telemetry internet setup</h1>
        <form action="./done" method="POST">
            <input id="ssid" name="ssid" placeholder="SSID"/>
            <input type="password" id="password" name="password" placeholder="Password"/>
            <input type="submit" value="Connect">
        </form>
    </div>
</body>
</html> <!-- end .html -->
)====";
}

const char* getClosedPage() {
    return R"====(
<html> <!-- original .html file -->
<head>
    <title>Configure password</title>
</head>
<style>
    html {
     font-family: Arial;
     text-align: center;
    }
    #wholeContainer{
        padding-top: 10px;
        padding-left: 10vw;
        padding-right: 10vw;
    }
    form {
        display: flex;
        flex-direction: column;
    }
    input{
        margin-bottom: 4px;
    }
  </style>
<body>
    <div id="wholeContainer">   
        <!-- <h1>Hi-horizon Telemetry internet setup</h1> -->
        <h2>Submission successful</h2>
        <h2>The MTU will try to connect to this network in 5 seconds</h2>
    </div>
</body>
</html> <!-- end .html -->
)====";
}