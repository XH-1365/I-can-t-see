from flask import Flask, request, render_template_string
from pypinyin import pinyin, Style
import requests

app = Flask(__name__)

ESP32_URL = "http://192.168.137.247/print"  # 改成你的

# 声母表（示例）
INITIALS = {
    "b": "12",
    "p": "1234",
    "m": "134",
    "f": "124",
    "d": "145",
    "t": "234",
    "n": "1345",
    "l": "123",
    "zh": "156",
    "ch": "16",
    "sh": "146",
    "r": "1245",
    "z": "135",
    "c": "1235",
    "s": "2345",
}

# 韵母表（示例）
FINALS = {
    "a": "1",
    "o": "13",
    "e": "15",
    "i": "2",
    "u": "3",
    "ai": "14",
    "ei": "25",
    "an": "124",
    "en": "125",
    "ang": "145",
    "eng": "245",
}

# 声调
TONES = {
    "1": "1",
    "2": "12",
    "3": "14",
    "4": "145",
    "5": ""  # 轻声
}

def split_pinyin(py):
    """拆分声母和韵母"""
    for i in range(2, 0, -1):
        if py[:i] in INITIALS:
            return py[:i], py[i:]
    return "", py

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

def chinese_to_code(text):
    pys = pinyin(text, style=Style.TONE3, neutral_tone_with_five=True)

    result = []

    for item in pys:
        py = item[0]
        result += pinyin_to_braille(py)

    return " ".join(result), pys

HTML = """
<h2>中文盲文打印</h2>
<form method="post">
<input name="text" style="width:300px;height:40px;font-size:20px">
<button type="submit">打印</button>
</form>
<pre>{{ result }}</pre>
"""

@app.route("/", methods=["GET","POST"])
def index():
    result = ""

    if request.method == "POST":
        text = request.form["text"]

        code, pys = chinese_to_code(text)

        result = f"""
输入: {text}
拼音: {pys}
盲文点位: {code}
"""

        try:
            requests.get(ESP32_URL, params={"code": code}, timeout=5)
            result += "\n已发送到打印机"
        except Exception as e:
            result += f"\n发送失败: {e}"

    return render_template_string(HTML, result=result)

if __name__ == "__main__":
    app.run(port=5000)