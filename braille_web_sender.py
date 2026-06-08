from flask import Flask, request, render_template_string
from pypinyin import pinyin, Style
import requests
import threading
import webbrowser

app = Flask(__name__)

# 你的 ESP32 地址
ESP32_BASE = "http://192.168.137.124"
ESP32_PRINT_URL = ESP32_BASE + "/print"
ESP32_FINISH_URL = ESP32_BASE + "/finish"

# 声母表
INITIALS = {
    "b": "12", "p": "1234", "m": "134", "f": "124",
    "d": "145", "t": "234", "n": "1345", "l": "123",
    "zh": "156", "ch": "16", "sh": "146", "r": "1245",
    "z": "135", "c": "1235", "s": "2345",
}

# 韵母表
FINALS = {
    "a": "1", "o": "13", "e": "15", "i": "2",
    "u": "3", "ai": "14", "ei": "25", "an": "124",
    "en": "125", "ang": "145", "eng": "245",
}

# 声调表
TONES = {
    "1": "1", "2": "12", "3": "14", "4": "145", "5": ""
}

# 拆分声母和韵母
def split_pinyin(py):
    for i in range(2, 0, -1):
        if py[:i] in INITIALS:
            return py[:i], py[i:]
    return "", py

# 拼音转盲文编码
def pinyin_to_braille(py):
    tone = py[-1] if py[-1].isdigit() else "5"
    base = py[:-1] if py[-1].isdigit() else py
    initial, final = split_pinyin(base)
    result = []
    if initial in INITIALS:
        result.append(INITIALS[initial])
    if final in FINALS:
        result.append(FINALS[final])
    if tone in TONES and TONES[tone]:
        result.append(TONES[tone])
    return result

# 中文文本转盲文编码
def chinese_to_code(text):
    pys = pinyin(text, style=Style.TONE3, neutral_tone_with_five=True)
    result = []
    for item in pys:
        py = item[0]
        result += pinyin_to_braille(py)
    return " ".join(result)

# 网页 HTML 模板
HTML = """
<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8">
<title>盲文打印机</title>
<style>
body{font-family:Arial;text-align:center;margin-top:50px;}
h1{font-size:40px;}
input{width:80%;max-width:600px;height:60px;font-size:24px;text-align:center;}
button{width:220px;height:60px;font-size:24px;margin:10px;cursor:pointer;}
.status{margin-top:30px;font-size:22px;}
</style>
</head>
<body>
<h1>盲文打印机</h1>
<form method="post">
<input name="text" placeholder="请输入中文" required><br><br>
<button type="submit">打印</button>
</form>
<form method="post" action="/finish">
<button type="submit">停止走纸</button>
</form>
<div class="status">{{ result }}</div>
</body>
</html>
"""

@app.route("/", methods=["GET", "POST"])
def index():
    result = "等待输入"
    if request.method == "POST":
        text = request.form["text"].strip()
        if text:
            code = chinese_to_code(text)
            try:
                r = requests.get(ESP32_PRINT_URL, params={"code": code}, timeout=5)
                result = f"正在打印：{text}"
            except Exception as e:
                result = f"发送失败：{e}"
    return render_template_string(HTML, result=result)

@app.route("/finish", methods=["GET", "POST"])
def finish():
    try:
        r = requests.get(ESP32_FINISH_URL, timeout=5)
        result = "已停止走纸"
    except Exception as e:
        result = f"请求失败：{e}"
    return render_template_string(HTML, result=result)

# 启动浏览器自动打开
def open_browser():
    webbrowser.open("http://127.0.0.1:5000")

if __name__ == "__main__":
    threading.Timer(1, open_browser).start()
    app.run(host="0.0.0.0", port=5000, debug=False)