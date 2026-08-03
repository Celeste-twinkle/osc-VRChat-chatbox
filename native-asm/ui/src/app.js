/* IIFE so the build step (esbuild) can mangle these names to short ones.
   Source stays readable; the shipped page gets the compact form. */
(function () {
const byId = (id) => document.getElementById(id),
  button = byId("button"),
  trButton = byId("trButton"),
  text = byId("text"),
  message = byId("message"),
  status = byId("status"),
  lan = byId("lan"),
  lanBtn = byId("lanBtn"),
  qrBtn = byId("qrBtn"),
  qrModal = byId("qrModal"),
  qrClose = byId("qrClose"),
  qrCanvas = byId("qrCanvas"),
  qrLink = byId("qrLink"),
  qrChoice = byId("qrChoice"),
  qrHost = byId("qrHost"),
  qrHostLabel = byId("qrHostLabel"),
  trOn = byId("trOn"),
  trBox = byId("trBox"),
  hint = byId("hint"),
  src = byId("src"),
  dst = byId("dst"),
  provider = byId("provider"),
  endpoint = byId("endpoint"),
  model = byId("model"),
  key = byId("key"),
  mmEmail = byId("mmEmail"),
  mmKey = byId("mmKey"),
  mmBox = byId("mmBox"),
  aiBox = byId("aiBox"),
  format = byId("fmt"),
  settingsSrc = byId("settingsSrc"),
  settingsDst = byId("settingsDst"),
  settingsFmt = byId("settingsFmt"),
  settingsSrcLabel = byId("settingsSrcLabel"),
  settingsDstLabel = byId("settingsDstLabel"),
  settingsFmtLabel = byId("settingsFmtLabel"),
  hlist = byId("hlist"),
  quickTr = byId("quickTr"),
  srcLabel = byId("srcLabel"),
  dstLabel = byId("dstLabel"),
  providerLabel = byId("providerLabel"),
  fmtLabel = byId("fmtLabel"),
  uiLang = byId("uiLang"),
  uiLangLabel = byId("uiLangLabel"),
  histLimit = byId("histLimit"),
  histLimitLabel = byId("histLimitLabel"),
  port = byId("port"),
  portLabel = byId("portLabel"),
  oscPort = byId("oscPort"),
  oscPortLabel = byId("oscPortLabel"),
  startup = byId("startup"),
  startMinimized = byId("startMinimized"),
  clearBtn = byId("clearBtn"),
  clearBubble = byId("clearBubble"),
  notifySfx = byId("notifySfx"),
  count = byId("cnt"),
  hbar = byId("hbar"),
  hnote = byId("hnote"),
  exportHistory = byId("exportHistory"),
  clearHistory = byId("clearHistory"),
  settingsBtn = byId("settingsBtn"),
  settingsModal = byId("settingsModal"),
  settingsClose = byId("settingsClose"),
  promptLabel = byId("promptLabel"),
  promptSelect = byId("promptSelect"),
  promptNew = byId("promptNew"),
  promptDelete = byId("promptDelete"),
  promptEditor = byId("promptEditor"),
  promptNameLabel = byId("promptNameLabel"),
  promptContentLabel = byId("promptContentLabel"),
  promptName = byId("promptName"),
  promptContent = byId("promptContent"),
  promptHint = byId("promptHint");
let timer = 0,
  promptTimer = 0,
  typingTimer = 0,
  history = [],
  hidCounter = 0,
  tapTimer = 0,
  pressBothTimer = 0,
  pressStart = 0,
  pressHid = -1,
  pressPart = "",
  pressEl = null,
  lastTapHid = -1,
  lastTapTime = 0,
  busy = false,
  lanAllowed = false,
  lanIps = [],
  serverPort = "", // learned from heartbeat X-Port header
  configuredPort = 19001,
  configuredOscPort = 9000,
  aiPrompts = [],
  activePromptId = "";
function validPort(v, fallback) {
  const n = Number(v);
  return Number.isInteger(n) && n >= 1024 && n <= 65535 ? n : fallback;
}
function curPort() {
  // When the page is viewed through a tunnel (a.natt.ctrla.top:25034) the
  // location.port is the tunnel port, not the server's real port. Prefer the
  // port reported by the server via the X-Port heartbeat header.
  return serverPort || location.port || "19001";
}
function normLanUrl(v) {
  v = (v || "").trim();
  if (!v) return "";
  if (/^https?:\/\//i.test(v)) return v;
  if (v.indexOf(":") < 0) v += ":" + curPort();
  return "http://" + v;
}
const historyKey = "vrcChatboxHistory",
  historyLimitKey = "vrcChatboxHistoryLimit",
  lanUrlKey = "vrcChatboxLanUrl";
const SYSTEM_PROMPT_ID = "",
  PROMPT_LIMIT = 6,
  PROMPT_CONTENT_LIMIT = 2500,
  DEFAULT_AI_PROMPT =
    "You are a translation engine. Translate the user text from {source} to {target}. Return only the translated text, with no quotes, labels, or commentary. If the input contains no translatable natural-language text or cannot be translated, return the original text unchanged.";
let translatableLetter;
try {
  translatableLetter = new RegExp("\\p{L}", "u");
} catch (e) {
  translatableLetter =
    /[A-Za-z\u00c0-\u02af\u0370-\u052f\u0590-\u1fff\u3040-\u30ff\u3400-\u9fff\uac00-\ud7af]/;
}
function hasTranslatableText(v) {
  return translatableLetter.test(v);
}
function cleanPrompt(p, i) {
  if (!p || typeof p !== "object") return null;
  let id = typeof p.id === "string" ? p.id.slice(0, 48) : "";
  if (!id || id === SYSTEM_PROMPT_ID) id = newPromptId();
  return {
    id: id,
    name: String(p.name || "").slice(0, 48) || L("promptUntitled") + " " + (i + 1),
    content: String(p.content || "").slice(0, PROMPT_CONTENT_LIMIT),
  };
}
function newPromptId() {
  return (
    "p" +
    Date.now().toString(36) +
    Math.random().toString(36).slice(2, 8)
  ).slice(0, 48);
}
function activePrompt() {
  for (let i = 0; i < aiPrompts.length; i++)
    if (aiPrompts[i].id === activePromptId) return aiPrompts[i];
  return null;
}
function renderPromptManager() {
  const selected = activePrompt();
  promptSelect.innerHTML = "";
  promptSelect.add(new Option(L("systemPrompt"), SYSTEM_PROMPT_ID));
  for (let i = 0; i < aiPrompts.length; i++)
    promptSelect.add(
      new Option(aiPrompts[i].name || L("promptUntitled"), aiPrompts[i].id),
    );
  if (!selected) activePromptId = SYSTEM_PROMPT_ID;
  promptSelect.value = activePromptId;
  const p = activePrompt();
  promptEditor.className = p ? "prompt-editor" : "prompt-editor hide";
  promptName.value = p ? p.name : "";
  promptContent.value = p ? p.content : "";
  promptDelete.disabled = !p;
  promptNew.disabled = aiPrompts.length >= PROMPT_LIMIT;
  promptHint.textContent =
    (p ? L("promptHintCustom") : L("promptHintDefault")) +
    (aiPrompts.length >= PROMPT_LIMIT ? " " + L("promptLimit") : "");
}
function resolvedPrompt() {
  const p = activePrompt(),
    template = p && p.content.trim() ? p.content : DEFAULT_AI_PROMPT;
  return template
    .replace(/\{source\}/g, src.value)
    .replace(/\{target\}/g, dst.value);
}
function getHistoryLimit() {
  let n = parseInt(
    localStorage.getItem(historyLimitKey) || histLimit.value || "100",
    10,
  );
  if (!isFinite(n) || n < 1) n = 100;
  if (n > 200) n = 200;
  histLimit.value = n;
  return n;
}
settingsSrc.innerHTML = src.innerHTML;
settingsDst.innerHTML = dst.innerHTML;
settingsFmt.innerHTML = format.innerHTML;
function syncSettingsFromQuick() {
  settingsSrc.value = src.value;
  settingsDst.value = dst.value;
  settingsFmt.value = format.value;
}
function syncQuickFromSettings() {
  src.value = settingsSrc.value;
  dst.value = settingsDst.value;
  format.value = settingsFmt.value;
}
const I18N = {
  zh: {
    connected: "本地服务已连接",
    lanTitle: "局域网访问地址",
    lanNote: "可在同一路由其他设备访问，如连接路由器wifi的手机",
    localOnly: "仅本机",
    settings: "设置",
    qr: "二维码",
    allowLan: "允许局域网连接",
    lanQr: "局域网二维码",
    close: "关闭",
    theme: "主题",
    startup: "开机自启动",
    startMinimized: "最小化自启动（开机启动时不打开浏览器）",
    enableTranslate: "启用翻译",
    uiLang: "UI 语言",
    srcLang: "源语言",
    dstLang: "目标语言",
    provider: "翻译服务",
    warn: "Key 会保存到本机 settings.json。不要把这个文件复制或发送给任何人。若 MyMemory 翻译失败或提示额度不足，请尝试填写 email，或自行获取 key 后使用。",
    mmEmail: "MyMemory email，可提升免费额度",
    mmKey: "MyMemory key，可选",
    mmKeyLink: "获取 MyMemory key",
    aiEndpoint: "AI Base URL，例如 https://api.openai.com/v1",
    aiModel: "模型，例如 gpt-4o-mini / deepseek-chat",
    aiKey: "AI API Key",
    aiPrompt: "AI 提示词",
    systemPrompt: "系统默认提示词",
    newPrompt: "新建",
    deletePrompt: "删除",
    promptName: "提示词名称",
    promptContent: "提示词内容",
    promptNamePlaceholder: "例如：简洁翻译",
    promptContentPlaceholder: "输入发送给 AI 的系统提示词",
    promptHintDefault:
      "系统默认提示词会处理当前语言，并在没有可翻译文本时原样返回。",
    promptHintCustom:
      "支持 {source} 和 {target} 占位符。空内容会回退到系统默认提示词。",
    promptUntitled: "未命名提示词",
    promptLimit: "最多可保存 6 个提示词。",
    promptDeleteConfirm: "删除当前提示词？",
    format: "翻译格式",
    fmtBoth: "原文 + 译文",
    fmtTrans: "仅译文",
    fmtOrig: "仅原文（不翻译）",
    send: "发送",
    directSend: "直接发送",
    translateSend: "翻译发送",
    clear: "清空",
    clearHistory: "清空历史",
    deleteHistory: "删除历史项",
    clearHistoryConfirm: "确定清空全部历史记录？",
    historyCleared: "历史记录已清空。",
    enterSend: "Enter 发送，Shift + Enter 换行",
    enterAction: "Enter {action}，Shift + Enter 换行",
    phDirect: "输入要直接发送到 VRChat Chatbox 的文字。",
    phOrig: "输入要发送的文字（不翻译）。",
    phTrans: "输入源语言。发送时仅发送译文。",
    phBoth: "输入源语言。发送时会把译文换行拼接到源语言后面。",
    lanFail: "开启失败",
    allowed: "已允许",
    retry: "重试",
    opening: "正在开启",
    lanOk: "已允许局域网连接",
    lanBad: "局域网连接开启失败",
    empty: "请输入内容后再发送。",
    translating: "翻译中...",
    sending: "发送中...",
    sentTrans: "已翻译并发送到 VRChat。",
    sent: "已发送到 VRChat。",
    sentStatus: "刚刚发送成功",
    missingAI: "请先填写 AI endpoint、model 和 API Key。",
    emptyTrans: "翻译失败或返回为空，请检查 API 配置或切换格式。",
    transFail:
      "翻译失败或发送失败。若使用 MyMemory，请尝试填写 email，或点击按钮自行获取 key 后使用。",
    sendFail: "发送失败，请确认 VRChat OSC 已开启。",
    badConn: "连接异常",
    historyFilled: "已填入历史消息，修改后按 Enter 发送",
    qrAddress: "二维码地址",
    qrSaved: "已保存",
    qrAuto: "自动检测",
    qrCurrent: "当前页面地址",
    qrCustom: "手动输入",
    historyLimit: "历史记录上限",
    port: "HTTP 端口（保存后生效）",
    oscPort: "OSC 端口（9000）",
    historyResendReady:
      "已填入历史发送内容，可编辑后手动发送",
    exportHistory: "导出历史",
    historyTapHint:
      "单击填入对应栏（原文/译文），双击填入两者；短按发送对应栏，长按（进度条满）一起发送。",
    resending: "重发中...",
    resent: "已重发到 VRChat。",
    resentStatus: "刚刚重发成功",
    resendFail: "重发失败，请确认 VRChat OSC 已开启。",
  },
  en: {
    connected: "Local service connected",
    lanTitle: "LAN access URL",
    lanNote: "Use another device on the same router, such as a phone on Wi-Fi.",
    localOnly: "Local only",
    settings: "Settings",
    qr: "QR",
    allowLan: "Allow LAN access",
    lanQr: "LAN QR code",
    close: "Close",
    theme: "Theme",
    startup: "Start with Windows",
    startMinimized: "Start minimized (do not open browser on startup)",
    enableTranslate: "Enable translation",
    uiLang: "UI language",
    srcLang: "Source language",
    dstLang: "Target language",
    provider: "Translation provider",
    warn: "Keys are saved locally in settings.json. Do not copy or share this file. If MyMemory fails or quota is low, add an email or use your own key.",
    mmEmail: "MyMemory email for higher free quota",
    mmKey: "MyMemory key, optional",
    mmKeyLink: "Get MyMemory key",
    aiEndpoint: "AI Base URL, e.g. https://api.openai.com/v1",
    aiModel: "Model, e.g. gpt-4o-mini / deepseek-chat",
    aiKey: "AI API Key",
    aiPrompt: "AI prompt",
    systemPrompt: "System default prompt",
    newPrompt: "New",
    deletePrompt: "Delete",
    promptName: "Prompt name",
    promptContent: "Prompt content",
    promptNamePlaceholder: "For example: Concise translation",
    promptContentPlaceholder: "Enter the system prompt sent to the AI",
    promptHintDefault:
      "The system default uses the current languages and returns input unchanged when there is no translatable text.",
    promptHintCustom:
      "Use {source} and {target} placeholders. Empty content falls back to the system default.",
    promptUntitled: "Untitled prompt",
    promptLimit: "You can save up to 6 prompts.",
    promptDeleteConfirm: "Delete the current prompt?",
    format: "Send format",
    fmtBoth: "Original + translation",
    fmtTrans: "Translation only",
    fmtOrig: "Original only (no translation)",
    send: "Send",
    directSend: "Direct send",
    translateSend: "Translate + send",
    clear: "Clear",
    clearHistory: "Clear history",
    deleteHistory: "Delete history item",
    clearHistoryConfirm: "Clear all history?",
    historyCleared: "History cleared.",
    enterSend: "Enter to send, Shift + Enter for newline",
    enterAction: "Enter to {action}, Shift + Enter for newline",
    phDirect: "Type text to send directly to VRChat Chatbox.",
    phOrig: "Type text to send without translation.",
    phTrans: "Type source text. Only the translation will be sent.",
    phBoth: "Type source text. Translation will be appended on the next line.",
    lanFail: "Failed to enable",
    allowed: "Allowed",
    retry: "Retry",
    opening: "Enabling",
    lanOk: "LAN access allowed",
    lanBad: "Failed to enable LAN access",
    empty: "Type something before sending.",
    translating: "Translating...",
    sending: "Sending...",
    sentTrans: "Translated and sent to VRChat.",
    sent: "Sent to VRChat.",
    sentStatus: "Sent just now",
    missingAI: "Fill in AI endpoint, model, and API key first.",
    emptyTrans:
      "Translation failed or returned empty. Check API settings or switch format.",
    transFail:
      "Translation or sending failed. If using MyMemory, try adding an email or your own key.",
    sendFail: "Send failed. Make sure VRChat OSC is enabled.",
    badConn: "Connection issue",
    historyFilled:
      "History message restored. Edit it, then press Enter to send.",
    qrAddress: "QR address",
    qrSaved: "Saved",
    qrAuto: "Auto detected",
    qrCurrent: "Current page URL",
    qrCustom: "Manual input",
    historyLimit: "History limit",
    port: "HTTP port (applies after save)",
    oscPort: "OSC port (9000)",
    historyResendReady:
      "History content restored. Edit if needed, then send manually.",
    exportHistory: "Export history",
    historyTapHint:
      "Tap a column to fill it (source/translation); double-tap fills both. Short press sends that column; long press (full bar) sends both.",
    resending: "Resending...",
    resent: "Resent to VRChat.",
    resentStatus: "Resent just now",
    resendFail: "Resend failed. Make sure VRChat OSC is enabled.",
  },
  ja: {
    connected: "ローカルサービスに接続済み",
    lanTitle: "LANアクセスURL",
    lanNote:
      "同じルーター上の端末、たとえばWi-Fi接続のスマートフォンからアクセスできます。",
    localOnly: "このPCのみ",
    settings: "設定",
    qr: "QR",
    allowLan: "LAN接続を許可",
    lanQr: "LAN QRコード",
    close: "閉じる",
    theme: "テーマ",
    startup: "Windows起動時に開始",
    startMinimized: "最小化起動（自動起動時にブラウザーを開かない）",
    enableTranslate: "翻訳を有効化",
    uiLang: "UI言語",
    srcLang: "元の言語",
    dstLang: "翻訳先言語",
    provider: "翻訳サービス",
    warn: "キーはローカルの settings.json に保存されます。このファイルをコピー、共有しないでください。MyMemoryが失敗する場合や上限に近い場合は、メールアドレスまたは自分のキーを設定してください。",
    mmEmail: "MyMemory email（無料枠を増やす）",
    mmKey: "MyMemory key（任意）",
    mmKeyLink: "MyMemory keyを取得",
    aiEndpoint: "AI Base URL 例: https://api.openai.com/v1",
    aiModel: "モデル 例: gpt-4o-mini / deepseek-chat",
    aiKey: "AI API Key",
    aiPrompt: "AIプロンプト",
    systemPrompt: "システム既定のプロンプト",
    newPrompt: "新規",
    deletePrompt: "削除",
    promptName: "プロンプト名",
    promptContent: "プロンプト内容",
    promptNamePlaceholder: "例：簡潔な翻訳",
    promptContentPlaceholder: "AIに送信するシステムプロンプト",
    promptHintDefault:
      "システム既定では現在の言語を使用し、翻訳できる文字がない場合は原文を返します。",
    promptHintCustom:
      "{source} と {target} を使用できます。空の場合はシステム既定に戻ります。",
    promptUntitled: "無題のプロンプト",
    promptLimit: "保存できるプロンプトは6件までです。",
    promptDeleteConfirm: "現在のプロンプトを削除しますか？",
    format: "送信形式",
    fmtBoth: "原文 + 翻訳",
    fmtTrans: "翻訳のみ",
    fmtOrig: "原文のみ（翻訳しない）",
    send: "送信",
    directSend: "直接送信",
    translateSend: "翻訳して送信",
    clear: "クリア",
    clearHistory: "履歴をクリア",
    deleteHistory: "履歴項目を削除",
    clearHistoryConfirm: "履歴をすべてクリアしますか？",
    historyCleared: "履歴をクリアしました。",
    enterSend: "Enterで送信、Shift + Enterで改行",
    enterAction: "Enterで{action}、Shift + Enterで改行",
    phDirect: "VRChat Chatbox に直接送信する文字を入力します。",
    phOrig: "翻訳せず送信する文字を入力します。",
    phTrans: "元の言語で入力します。送信時は翻訳のみ送ります。",
    phBoth: "元の言語で入力します。翻訳を次の行に追加して送信します。",
    lanFail: "有効化に失敗",
    allowed: "許可済み",
    retry: "再試行",
    opening: "有効化中",
    lanOk: "LAN接続を許可しました",
    lanBad: "LAN接続の有効化に失敗しました",
    empty: "内容を入力してから送信してください。",
    translating: "翻訳中...",
    sending: "送信中...",
    sentTrans: "翻訳してVRChatへ送信しました。",
    sent: "VRChatへ送信しました。",
    sentStatus: "送信しました",
    missingAI: "AI endpoint、model、API Key を先に入力してください。",
    emptyTrans:
      "翻訳に失敗したか、空の結果です。API設定または形式を確認してください。",
    transFail:
      "翻訳または送信に失敗しました。MyMemoryを使う場合はメールまたはキーを設定してください。",
    sendFail: "送信に失敗しました。VRChat OSC が有効か確認してください。",
    badConn: "接続エラー",
    historyFilled: "履歴を入力欄に戻しました。編集してEnterで送信できます。",
    qrAddress: "QRアドレス",
    qrSaved: "保存済み",
    qrAuto: "自動検出",
    qrCurrent: "現在のページURL",
    qrCustom: "手動入力",
    historyLimit: "履歴の上限",
    port: "HTTPポート（保存後に有効）",
    oscPort: "OSCポート（9000）",
    historyResendReady: "履歴の内容を入力欄に戻しました。必要なら編集して手動で送信してください。",
    exportHistory: "履歴をエクスポート",
    historyTapHint:
      "タップで対応する欄（原文/翻訳）を入力、ダブルタップで両方入力。短押しでその欄を送信、長押し（バー満タン）で両方送信。",
    resending: "再送信中...",
    resent: "VRChatへ再送信しました。",
    resentStatus: "再送信しました",
    resendFail: "再送信に失敗しました。VRChat OSC が有効か確認してください。",
  },
  ko: {
    connected: "로컬 서비스 연결됨",
    lanTitle: "LAN 접속 주소",
    lanNote:
      "같은 라우터의 다른 기기, 예를 들어 Wi-Fi에 연결된 휴대폰에서 접속할 수 있습니다.",
    localOnly: "이 PC만",
    settings: "설정",
    qr: "QR",
    allowLan: "LAN 접속 허용",
    lanQr: "LAN QR 코드",
    close: "닫기",
    theme: "테마",
    startup: "Windows 시작 시 실행",
    startMinimized: "최소화 시작(자동 시작 시 브라우저 열지 않음)",
    enableTranslate: "번역 사용",
    uiLang: "UI 언어",
    srcLang: "원본 언어",
    dstLang: "대상 언어",
    provider: "번역 서비스",
    warn: "키는 로컬 settings.json에 저장됩니다. 이 파일을 복사하거나 공유하지 마세요. MyMemory가 실패하거나 한도가 부족하면 email 또는 개인 key를 설정하세요.",
    mmEmail: "MyMemory email, 무료 한도 증가",
    mmKey: "MyMemory key, 선택 사항",
    mmKeyLink: "MyMemory key 받기",
    aiEndpoint: "AI Base URL 예: https://api.openai.com/v1",
    aiModel: "모델 예: gpt-4o-mini / deepseek-chat",
    aiKey: "AI API Key",
    aiPrompt: "AI 프롬프트",
    systemPrompt: "시스템 기본 프롬프트",
    newPrompt: "새로 만들기",
    deletePrompt: "삭제",
    promptName: "프롬프트 이름",
    promptContent: "프롬프트 내용",
    promptNamePlaceholder: "예: 간결한 번역",
    promptContentPlaceholder: "AI에 보낼 시스템 프롬프트",
    promptHintDefault:
      "시스템 기본값은 현재 언어를 사용하며 번역할 텍스트가 없으면 원문을 반환합니다.",
    promptHintCustom:
      "{source} 및 {target} 자리표시자를 사용할 수 있습니다. 비어 있으면 시스템 기본값을 사용합니다.",
    promptUntitled: "이름 없는 프롬프트",
    promptLimit: "프롬프트는 최대 6개까지 저장할 수 있습니다.",
    promptDeleteConfirm: "현재 프롬프트를 삭제할까요?",
    format: "전송 형식",
    fmtBoth: "원문 + 번역",
    fmtTrans: "번역만",
    fmtOrig: "원문만(번역 안 함)",
    send: "전송",
    directSend: "직접 전송",
    translateSend: "번역 후 전송",
    clear: "지우기",
    clearHistory: "히스토리 지우기",
    deleteHistory: "히스토리 항목 삭제",
    clearHistoryConfirm: "히스토리를 모두 지우시겠습니까?",
    historyCleared: "히스토리를 지웠습니다.",
    enterSend: "Enter 전송, Shift + Enter 줄바꿈",
    enterAction: "Enter {action}, Shift + Enter 줄바꿈",
    phDirect: "VRChat Chatbox로 바로 보낼 문장을 입력하세요.",
    phOrig: "번역하지 않고 보낼 문장을 입력하세요.",
    phTrans: "원본 언어로 입력하세요. 전송 시 번역만 보냅니다.",
    phBoth: "원본 언어로 입력하세요. 번역을 다음 줄에 붙여 보냅니다.",
    lanFail: "활성화 실패",
    allowed: "허용됨",
    retry: "다시 시도",
    opening: "활성화 중",
    lanOk: "LAN 접속을 허용했습니다",
    lanBad: "LAN 접속 활성화 실패",
    empty: "내용을 입력한 뒤 전송하세요.",
    translating: "번역 중...",
    sending: "전송 중...",
    sentTrans: "번역 후 VRChat에 전송했습니다.",
    sent: "VRChat에 전송했습니다.",
    sentStatus: "방금 전송됨",
    missingAI: "AI endpoint, model, API Key를 먼저 입력하세요.",
    emptyTrans: "번역 실패 또는 빈 결과입니다. API 설정이나 형식을 확인하세요.",
    transFail:
      "번역 또는 전송에 실패했습니다. MyMemory 사용 시 email 또는 개인 key를 설정해 보세요.",
    sendFail: "전송 실패. VRChat OSC가 켜져 있는지 확인하세요.",
    badConn: "연결 오류",
    historyFilled:
      "히스토리 메시지를 입력창에 넣었습니다. 수정 후 Enter로 전송하세요.",
    qrAddress: "QR 주소",
    qrSaved: "저장됨",
    qrAuto: "자동 감지",
    qrCurrent: "현재 페이지 URL",
    qrCustom: "수동 입력",
    historyLimit: "히스토리 상한",
    port: "HTTP 포트(저장 후 적용)",
    oscPort: "OSC 포트(9000)",
    historyResendReady:
      "히스토리 내용을 입력창에 넣었습니다. 필요하면 수정 후 수동으로 전송하세요.",
    exportHistory: "히스토리 내보내기",
    historyTapHint:
      "클릭하면 해당 칸(원문/번역) 입력, 더블클릭하면 둘 다 입력. 짧게 누르면 해당 칸 전송, 길게(진행바 가득) 누르면 둘 다 전송.",
    resending: "재전송 중...",
    resent: "VRChat에 재전송했습니다.",
    resentStatus: "방금 재전송됨",
    resendFail: "재전송 실패. VRChat OSC가 켜져 있는지 확인하세요.",
  },
};
let lang = "zh";
function pickLang(v) {
  let n = (v && v !== "auto" ? v : navigator.language || "zh").toLowerCase();
  if (n.startsWith("ja")) return "ja";
  if (n.startsWith("ko")) return "ko";
  if (n.startsWith("en")) return "en";
  return "zh";
}
function L(k) {
  return (I18N[lang] && I18N[lang][k]) || I18N.zh[k] || k;
}
function tx(el, k) {
  if (el) el.textContent = L(k);
}
function opt(el, i, k) {
  if (el && el.options[i]) el.options[i].textContent = L(k);
}
function lab(input, k) {
  if (!input) return;
  let n = input.nextSibling;
  if (n) n.nodeValue = L(k);
}
function applyLang() {
  lang = pickLang(uiLang.value);
  document.documentElement.lang = lang === "zh" ? "zh-CN" : lang;
  tx(status, "connected");
  tx(document.querySelector(".lan b"), "lanTitle");
  tx(document.querySelector(".lan-note"), "lanNote");
  tx(settingsBtn, "settings");
  tx(qrBtn, "qr");
  tx(document.querySelector("#qrModal .ph b"), "lanQr");
  tx(qrClose, "close");
  tx(qrHostLabel, "qrAddress");
  tx(document.querySelector("#settingsModal .ph b"), "settings");
  tx(settingsClose, "close");
  tx(uiLangLabel, "uiLang");
  tx(histLimitLabel, "historyLimit");
  tx(portLabel, "port");
  tx(oscPortLabel, "oscPort");
  lab(startup, "startup");
  lab(startMinimized, "startMinimized");
  lab(trOn, "enableTranslate");
  tx(srcLabel, "srcLang");
  tx(dstLabel, "dstLang");
  tx(settingsSrcLabel, "srcLang");
  tx(settingsDstLabel, "dstLang");
  tx(settingsFmtLabel, "format");
  tx(providerLabel, "provider");
  tx(document.querySelector(".warn"), "warn");
  mmEmail.placeholder = L("mmEmail");
  mmKey.placeholder = L("mmKey");
  tx(document.querySelector(".linkbtn"), "mmKeyLink");
  endpoint.placeholder = L("aiEndpoint");
  model.placeholder = L("aiModel");
  key.placeholder = L("aiKey");
  tx(promptLabel, "aiPrompt");
  tx(promptNew, "newPrompt");
  tx(promptDelete, "deletePrompt");
  tx(promptNameLabel, "promptName");
  tx(promptContentLabel, "promptContent");
  promptName.placeholder = L("promptNamePlaceholder");
  promptContent.placeholder = L("promptContentPlaceholder");
  promptName.setAttribute("aria-label", L("promptName"));
  promptContent.setAttribute("aria-label", L("promptContent"));
  tx(fmtLabel, "format");
  opt(format, 0, "fmtBoth");
  opt(format, 1, "fmtTrans");
  opt(format, 2, "fmtOrig");
  opt(settingsFmt, 0, "fmtBoth");
  opt(settingsFmt, 1, "fmtTrans");
  opt(settingsFmt, 2, "fmtOrig");
  tx(clearBtn, "clear");
  tx(exportHistory, "exportHistory");
  tx(hnote, "historyTapHint");
  renderPromptManager();
  setLanState(qrBtn.dataset.ip || "127.0.0.1", qrBtn.dataset.fail === "1");
  showBoxes();
  if (history.length) renderHistory();
}
const sid = (
  Date.now().toString(36) + Math.random().toString(36).slice(2, 10)
).slice(0, 31);
function isLocalHost() {
  var h = location.hostname;
  return h === "127.0.0.1" || h === "localhost" || h === "::1" || h === "";
}
function beat() {
  var url = "/heartbeat?id=" + encodeURIComponent(sid);
  fetch(url, { method: "POST" })
    .then(function (r) {
      var p = r.headers.get("X-Port");
      if (p && p !== serverPort) {
        serverPort = p;
        // Refresh the LAN address shown: when viewed through a tunnel the
        // page port differs from the real server port.
        if (qrBtn.dataset.ip) {
          var on = qrBtn.dataset.fail !== "1" && qrBtn.dataset.ip !== "127.0.0.1";
          if (on) setLanState(qrBtn.dataset.ip, false, lanIps);
          if (qrModal.className === "modal") renderLanChoices(qrBtn.dataset.ip);
        }
      }
      // Only local clients can follow the X-Port redirect: a tunnel/proxy
      // client would break (the tunnel maps only the old port).
      if (isLocalHost() && p && p !== (location.port || "80")) {
        location.href =
          location.protocol + "//" + location.hostname + ":" + p + "/";
        return;
      }
      if (!busy) {
        status.textContent = L("connected");
        status.className = "s";
      }
    })
    .catch(function () {
      if (!busy) {
        status.textContent = L("badConn");
        status.className = "s e";
      }
    });
}
beat();
setInterval(beat, 3000);
function setTyping(on) {
  // Single request per state change: sendBeacon + fetch would double-send
  // the "false" close signal on every stop-typing transition.
  if (!on) {
    try {
      if (navigator.sendBeacon && navigator.sendBeacon("/typing", "false"))
        return;
    } catch (e) {}
  }
  fetch("/typing", {
    method: "POST",
    body: on ? "true" : "false",
    keepalive: true,
  }).catch(function () {});
}
function sendTyping() {
  // Only report "typing" while there is actual text: focus alone (e.g. after
  // a button click refocuses the box for the next input) must not keep the
  // typing indicator alive.
  setTyping(!!text.value.trim());
}
window.addEventListener("pagehide", function () {
  setTyping(false);
});
function openSettings() {
  settingsModal.className = "modal";
}
function closeSettings() {
  settingsModal.className = "modal hide";
  text.focus();
}
settingsBtn.addEventListener("click", openSettings);
settingsClose.addEventListener("click", closeSettings);
settingsModal.addEventListener("click", function (e) {
  if (e.target === settingsModal) closeSettings();
});
function addLanChoice(a, u, label) {
  u = normLanUrl(u);
  if (!u) return;
  for (let i = 0; i < a.length; i++) if (a[i].url === u) return;
  a.push({ url: u, label: label });
}
function lanChoiceList(ip) {
  let a = [],
    saved = localStorage.getItem(lanUrlKey) || "";
  if (/^https?:\/\/0\./i.test(saved)) {
    localStorage.removeItem(lanUrlKey);
    saved = "";
  }
  // "Current page URL" always comes first and is never deduped away: it is
  // the address the user is actually viewing (may be a tunnel/proxy host).
  if (location.origin) {
    if (normLanUrl(saved) === location.origin) saved = "";
    a.push({ url: location.origin, label: L("qrCurrent") });
  }
  addLanChoice(a, saved, L("qrSaved"));
  if (ip && ip !== "127.0.0.1")
    addLanChoice(a, "http://" + ip + ":" + curPort(), L("qrAuto"));
  (lanIps || []).forEach((x) =>
    addLanChoice(a, "http://" + x + ":" + curPort(), L("qrAuto")),
  );
  return a;
}
function escOpt(v) {
  return v.replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/"/g, "&quot;");
}
function renderLanChoices(ip) {
  let a = lanChoiceList(ip);
  qrChoice.innerHTML =
    a
      .map(
        (x) =>
          '<option value="' +
          escOpt(x.url) +
          '">' +
          escOpt(x.label + ": " + x.url) +
          "</option>",
      )
      .join("") +
    '<option value="__custom">' +
    L("qrCustom") +
    "</option>";
  let u =
    localStorage.getItem(lanUrlKey) ||
    qrBtn.dataset.url ||
    (a[0] && a[0].url) ||
    "";
  if (u) setQrUrl(u, false);
}
function setQrUrl(u, persist = true) {
  u = normLanUrl(u);
  if (!u) return;
  if (persist) localStorage.setItem(lanUrlKey, u);
  qrBtn.dataset.url = u;
  qrHost.value = u;
  drawQr(qrCanvas, u);
  qrLink.href = u;
  qrLink.textContent = u;
  let found = false;
  for (let i = 0; i < qrChoice.options.length; i++) {
    if (qrChoice.options[i].value === u) {
      qrChoice.value = u;
      found = true;
      break;
    }
  }
  if (!found) qrChoice.value = "__custom";
}
function openQr() {
  let u = qrBtn.dataset.url;
  if (!u) return;
  renderLanChoices(qrBtn.dataset.ip || "127.0.0.1");
  qrModal.className = "modal";
}
function closeQr() {
  qrModal.className = "modal hide";
  text.focus();
}
qrBtn.addEventListener("click", openQr);
qrChoice.addEventListener("change", function () {
  if (qrChoice.value === "__custom") {
    qrHost.focus();
    return;
  }
  setQrUrl(qrChoice.value);
});
qrHost.addEventListener("change", function () {
  setQrUrl(qrHost.value);
});
qrClose.addEventListener("click", closeQr);
qrModal.addEventListener("click", function (e) {
  if (e.target === qrModal) closeQr();
});
document.addEventListener("keydown", function (e) {
  if (e.key === "Escape") {
    closeSettings();
    closeQr();
  }
});
function setLanState(ip, fail, ips) {
  if (ips && ips.length) lanIps = ips;
  const on = !fail && ip !== "127.0.0.1";
  const u = on ? "http://" + ip + ":" + curPort() : "",
    saved = localStorage.getItem(lanUrlKey) || "",
    shown = saved || u;
  lanAllowed = on;
  qrBtn.dataset.ip = ip;
  qrBtn.dataset.fail = fail ? "1" : "0";
  lan.textContent = on ? shown : fail ? L("lanFail") : L("localOnly");
  qrBtn.className = on ? "" : "hide";
  qrBtn.dataset.url = shown;
  if (qrModal.className === "modal") renderLanChoices(ip);
  lanBtn.disabled = on;
  lanBtn.textContent = on ? L("allowed") : fail ? L("retry") : L("allowLan");
}
async function refreshLan() {
  try {
    const r = await fetch("/lan-ip");
    const j = await r.json();
    setLanState(j.ip, false, j.ips || []);
  } catch (e) {
    setLanState("127.0.0.1", false);
  }
}
async function enableLan() {
  lanBtn.disabled = true;
  lanBtn.textContent = L("opening");
  try {
    const r = await fetch("/lan-enable", { method: "POST" });
    const j = await r.json();
    const on = j.ip !== "127.0.0.1";
    setLanState(j.ip, !on, j.ips || []);
    if (on) await save();
    status.textContent = on ? L("lanOk") : L("lanBad");
  } catch (e) {
    setLanState("127.0.0.1", true);
  }
}
function drawQr(c, txt) {
  const N = 29,
    D = 55,
    E = 15;
  let bits = [0, 1, 0, 0];
  for (let i = 7; i >= 0; i--) bits.push((txt.length >> i) & 1);
  for (let ch of txt) {
    let v = ch.charCodeAt(0);
    for (let i = 7; i >= 0; i--) bits.push((v >> i) & 1);
  }
  for (let i = 0; i < 4 && bits.length < D * 8; i++) bits.push(0);
  while (bits.length % 8) bits.push(0);
  let data = [];
  for (let i = 0; i < bits.length; i += 8)
    data.push(bits.slice(i, i + 8).reduce((a, b) => a * 2 + b, 0));
  for (let p = 0xec; data.length < D; p = p == 0xec ? 0x11 : 0xec) data.push(p);
  function gm(a, b) {
    let r = 0;
    for (; b; b >>= 1) {
      if (b & 1) r ^= a;
      a <<= 1;
      if (a & 256) a ^= 0x11d;
    }
    return r;
  }
  function gp(n) {
    let r = 1;
    while (n--) r = gm(r, 2);
    return r;
  }
  let g = [1];
  for (let i = 0; i < E; i++) {
    let ng = Array(g.length + 1).fill(0),
      a = gp(i);
    for (let j = 0; j < g.length; j++) {
      ng[j] ^= g[j];
      ng[j + 1] ^= gm(g[j], a);
    }
    g = ng;
  }
  g = g.slice(1);
  let rem = Array(E).fill(0);
  for (let b of data) {
    let f = b ^ rem.shift();
    rem.push(0);
    for (let i = 0; i < E; i++) rem[i] ^= gm(g[i], f);
  }
  let cw = data.concat(rem),
    m = Array.from({ length: N }, () => Array(N).fill(-1));
  function set(x, y, v) {
    if (x >= 0 && y >= 0 && x < N && y < N) m[y][x] = v;
  }
  function finder(x, y) {
    for (let dy = -1; dy < 8; dy++)
      for (let dx = -1; dx < 8; dx++) {
        let v = 0;
        if (
          dx >= 0 &&
          dx < 7 &&
          dy >= 0 &&
          dy < 7 &&
          (dx == 0 ||
            dx == 6 ||
            dy == 0 ||
            dy == 6 ||
            (dx >= 2 && dx <= 4 && dy >= 2 && dy <= 4))
        )
          v = 1;
        set(x + dx, y + dy, v);
      }
  }
  function align(x, y) {
    for (let dy = -2; dy <= 2; dy++)
      for (let dx = -2; dx <= 2; dx++)
        set(x + dx, y + dy, Math.max(Math.abs(dx), Math.abs(dy)) != 1 ? 1 : 0);
  }
  finder(0, 0);
  finder(N - 7, 0);
  finder(0, N - 7);
  align(22, 22);
  for (let i = 8; i < N - 8; i++) {
    if (m[6][i] < 0) set(i, 6, i % 2 == 0);
    if (m[i][6] < 0) set(6, i, i % 2 == 0);
  }
  for (let i = 0; i < 8; i++) {
    set(N - 1 - i, 8, 0);
    set(8, N - 1 - i, 0);
  }
  for (let i = 0; i < 6; i++) {
    set(8, i, 0);
    set(i, 8, 0);
  }
  set(8, 7, 0);
  set(8, 8, 0);
  set(7, 8, 0);
  for (let i = 0; i < 6; i++) set(5 - i, 8, 0);
  set(8, 21, 1);
  let bi = 0;
  for (let x = N - 1, up = true; x > 0; x -= 2) {
    if (x == 6) x--;
    for (let yy = 0; yy < N; yy++) {
      let y = up ? N - 1 - yy : yy;
      for (let dx = 0; dx < 2; dx++) {
        let xx = x - dx;
        if (m[y][xx] < 0) {
          let bit =
            bi < cw.length * 8 ? (cw[bi >> 3] >> (7 - (bi & 7))) & 1 : 0;
          m[y][xx] = bit ^ ((xx + y) % 2 == 0);
          bi++;
        }
      }
    }
    up = !up;
  }
  let fmt = 0b111011111000100;
  for (let i = 0; i < 15; i++) {
    let v = (fmt >> i) & 1;
    if (i < 6) set(8, i, v);
    else if (i < 8) set(8, i + 1, v);
    else if (i == 8) set(7, 8, v);
    else set(14 - i, 8, v);
    if (i < 8) set(N - 1 - i, 8, v);
    else set(8, N - 15 + i, v);
  }
  let q = 4,
    sc = Math.floor(c.width / (N + q * 2)),
    ctx = c.getContext("2d");
  // QR codes must stay white-background/dark-modules regardless of theme:
  // scanners expect the standard contrast, and themed colors break on
  // print/screenshot (a dark-theme canvas turns into unreadable white bg).
  ctx.fillStyle = "#ffffff";
  ctx.fillRect(0, 0, c.width, c.height);
  ctx.fillStyle = "#111827";
  for (let y = 0; y < N; y++)
    for (let x = 0; x < N; x++)
      if (m[y][x]) ctx.fillRect((x + q) * sc, (y + q) * sc, sc, sc);
}
function showBoxes() {
  let on = trOn.checked,
    mm = provider.value === "mymemory",
    f = format.value,
    trg = on && f !== "orig"; // translation actually happens
  trBox.className = on ? "" : "hide";
  quickTr.className = on ? "quick" : "quick hide";
  mmBox.className = on && mm ? "row" : "row hide";
  aiBox.className = on && !mm ? "" : "hide";
  // Two always-predictable buttons: [直接发送] sends the box verbatim,
  // [翻译发送] translates it. When translation is off (or format is
  // orig-only) the translate button disappears so it can't mislead.
  trButton.className = trg ? "send-tr" : "send-tr hide";
  button.textContent = trg ? L("directSend") : L("send");
  // Direct-send colour follows its role: when translation is on it is a
  // secondary bypass (grey); when it is the only send button it becomes the
  // primary accent so it reads as the main action.
  button.className = trg ? "send-direct" : "send-direct primary";
  let lb = trg ? L("translateSend") : L("send");
  hint.textContent = on
    ? L("enterAction").replace("{action}", lb)
    : L("enterSend");
  text.placeholder = on
    ? f === "orig"
      ? L("phOrig")
      : f === "trans"
        ? L("phTrans")
        : L("phBoth")
    : L("phDirect");
}
function preset(force) {
  const ps = {
    openai: ["https://api.openai.com/v1", "gpt-4o-mini"],
    deepseek: ["https://api.deepseek.com", "deepseek-chat"],
    hunyuan: [
      "https://api.hunyuan.cloud.tencent.com/v1",
      "hunyuan-turbos-latest",
    ],
  }[provider.value];
  if (!ps) return;
  if (force || !endpoint.value) endpoint.value = ps[0];
  if (force || !model.value) model.value = ps[1];
}
function syncStartup() {
  startMinimized.disabled = !startup.checked;
  if (!startup.checked) startMinimized.checked = false;
}
async function loadPrompts() {
  try {
    const r = await fetch("/prompts"),
      j = await r.json(),
      raw = Array.isArray(j.items) ? j.items : [],
      seen = {};
    aiPrompts = raw
      .slice(0, PROMPT_LIMIT)
      .map(cleanPrompt)
      .filter(function (p) {
        if (!p || seen[p.id]) return false;
        seen[p.id] = true;
        return true;
      });
    activePromptId =
      typeof j.activeId === "string" &&
      aiPrompts.some((p) => p.id === j.activeId)
        ? j.activeId
        : SYSTEM_PROMPT_ID;
  } catch (e) {
    aiPrompts = [];
    activePromptId = SYSTEM_PROMPT_ID;
  }
  renderPromptManager();
}
async function savePrompts() {
  clearTimeout(promptTimer);
  const body = JSON.stringify({
    activeId: activePromptId,
    items: aiPrompts.slice(0, PROMPT_LIMIT),
  });
  try {
    const r = await fetch("/prompts", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: body,
    });
    if (!r.ok) throw Error();
  } catch (e) {}
}
function queuePromptSave() {
  clearTimeout(promptTimer);
  promptTimer = setTimeout(savePrompts, 600);
}
async function load() {
  try {
    const r = await fetch("/settings");
    const j = await r.json();
    trOn.checked = !!j.translate;
    src.value = j.src || src.value;
    dst.value = j.dst || dst.value;
    provider.value = j.provider || provider.value;
    endpoint.value = j.endpoint || "";
    model.value = j.model || "";
    key.value = j.key || "";
    mmEmail.value = j.mmEmail || "";
    mmKey.value = j.mmKey || "";
    format.value = j.format || format.value;
    notifySfx.checked = j.notifySfx !== false;
    syncSettingsFromQuick();
    uiLang.value = j.uiLang || "auto";
    lang = pickLang(uiLang.value);
    startup.checked = !!j.startup;
    startMinimized.checked = !!j.startMinimized;
    lanAllowed = !!j.lan;
    configuredPort = validPort(j.port, 19001);
    configuredOscPort = validPort(j.oscPort, 9000);
    port.value = configuredPort;
    oscPort.value = configuredOscPort;
    syncStartup();
    preset(false);
    applyLang();
  } catch (e) {
    syncStartup();
    applyLang();
  }
}
async function save() {
  clearTimeout(timer);
  syncStartup();
  const nextPort = validPort(port.value, configuredPort),
    nextOscPort = validPort(oscPort.value, configuredOscPort);
  port.value = nextPort;
  oscPort.value = nextOscPort;
  const j = {
    translate: trOn.checked,
    src: src.value,
    dst: dst.value,
    provider: provider.value,
    endpoint: endpoint.value,
    model: model.value,
    key: key.value,
    mmEmail: mmEmail.value,
    mmKey: mmKey.value,
    format: format.value,
    notifySfx: notifySfx.checked,
    uiLang: uiLang.value,
    startup: startup.checked,
    startMinimized: startMinimized.checked,
    lan: lanAllowed,
    port: nextPort,
    oscPort: nextOscPort,
  };
  try {
    const r = await fetch("/settings", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify(j),
    });
    if (!r.ok) throw Error();
    configuredPort = nextPort;
    configuredOscPort = nextOscPort;
  } catch (e) {}
}
uiLang.addEventListener("change", () => {
  applyLang();
  save();
});
histLimit.addEventListener("change", () => {
  localStorage.setItem(historyLimitKey, getHistoryLimit());
  trimHistory();
  saveHistory();
});
notifySfx.addEventListener("change", function () {
  fetch("/notify-sfx", {
    method: "POST",
    body: notifySfx.checked ? "true" : "false",
  }).catch(function () {});
  save();
});
trOn.addEventListener("change", () => {
  showBoxes();
  save();
});
startup.addEventListener("change", () => {
  syncStartup();
  save();
});
startMinimized.addEventListener("change", save);
function quickTranslateChanged() {
  syncSettingsFromQuick();
  showBoxes();
  save();
  text.focus();
}
function settingsTranslateChanged() {
  syncQuickFromSettings();
  showBoxes();
  save();
}
[src, dst, format].forEach((x) =>
  x.addEventListener("change", quickTranslateChanged),
);
[settingsSrc, settingsDst, settingsFmt].forEach((x) =>
  x.addEventListener("change", settingsTranslateChanged),
);
provider.addEventListener("change", () => {
  preset(true);
  showBoxes();
  save();
});
[endpoint, model, key, mmEmail, mmKey, port, oscPort].forEach((x) =>
  x.addEventListener("change", save),
);
[key, endpoint, model, mmEmail, mmKey].forEach((x) =>
  x.addEventListener("input", () => {
    clearTimeout(timer);
    timer = setTimeout(save, 600);
  }),
);
promptSelect.addEventListener("change", function () {
  activePromptId = promptSelect.value;
  renderPromptManager();
  savePrompts();
});
promptNew.addEventListener("click", function () {
  if (aiPrompts.length >= PROMPT_LIMIT) {
    promptHint.textContent = L("promptLimit");
    return;
  }
  const p = {
    id: newPromptId(),
    name: L("promptUntitled") + " " + (aiPrompts.length + 1),
    content: DEFAULT_AI_PROMPT,
  };
  aiPrompts.push(p);
  activePromptId = p.id;
  renderPromptManager();
  promptName.focus();
  promptName.select();
  savePrompts();
});
promptDelete.addEventListener("click", function () {
  const p = activePrompt();
  if (!p || !confirm(L("promptDeleteConfirm"))) return;
  aiPrompts = aiPrompts.filter((item) => item.id !== p.id);
  activePromptId = SYSTEM_PROMPT_ID;
  renderPromptManager();
  savePrompts();
});
promptName.addEventListener("input", function () {
  const p = activePrompt();
  if (!p) return;
  p.name = promptName.value.slice(0, 48);
  const option = Array.from(promptSelect.options).find((o) => o.value === p.id);
  if (option) option.textContent = p.name || L("promptUntitled");
  queuePromptSave();
});
promptContent.addEventListener("input", function () {
  const p = activePrompt();
  if (!p) return;
  p.content = promptContent.value.slice(0, PROMPT_CONTENT_LIMIT);
  queuePromptSave();
});
async function tr(v) {
  if (provider.value !== "mymemory") return trAI(v);
  let u =
    "https://api.mymemory.translated.net/get?q=" +
    encodeURIComponent(v) +
    "&langpair=" +
    encodeURIComponent(src.value + "|" + dst.value);
  if (mmEmail.value) u += "&de=" + encodeURIComponent(mmEmail.value);
  if (mmKey.value) u += "&key=" + encodeURIComponent(mmKey.value);
  let r = await fetch(u);
  let j = await r.json();
  return j.responseData && j.responseData.translatedText
    ? j.responseData.translatedText
    : "";
}
function chatUrl() {
  let u = endpoint.value.trim().replace(/\/+$/, "");
  return u.endsWith("/chat/completions") ? u : u + "/chat/completions";
}
async function trAI(v) {
  if (!endpoint.value || !model.value || !key.value)
    throw Error("missing ai settings");
  let r = await fetch(chatUrl(), {
    method: "POST",
    headers: {
      "Content-Type": "application/json",
      Authorization: "Bearer " + key.value,
    },
    body: JSON.stringify({
      model: model.value,
      messages: [
        {
          role: "system",
          content: resolvedPrompt(),
        },
        { role: "user", content: v },
      ],
    }),
  });
  let j = await r.json();
  return j.choices && j.choices[0] && j.choices[0].message
    ? j.choices[0].message.content
    : "";
}
async function sendText(direct) {
  if (busy) return;
  const v = text.value.trim();
  if (!v) {
    message.textContent = L("empty");
    message.className = "m e";
    text.focus();
    return;
  }
  const canTranslate = hasTranslatableText(v);
  busy = true;
  button.disabled = true;
  trButton.disabled = true;
  message.textContent =
    direct || !trOn.checked || !canTranslate
      ? L("sending")
      : L("translating");
  message.className = "m";
  try {
    await save();
    let tv = "",
      out = "";
    if (
      direct ||
      !trOn.checked ||
      format.value === "orig" ||
      !canTranslate
    ) {
      // Direct send: the box content goes out verbatim. This also covers
      // translation-off, orig-only formats, and inputs with no letters.
      out = v;
    } else {
      tv = await tr(v);
      if (!tv) throw Error("empty trans");
      if (format.value === "trans") out = tv;
      else out = v + "\n" + tv;
    }
    const r = await fetch("/send", {
      method: "POST",
      headers: { "Content-Type": "text/plain;charset=utf-8" },
      body: out,
    });
    if (!r.ok) throw Error();
    var h = {
      hid: ++hidCounter,
      text: v,
      trans: tv,
      src: src.value,
      dst: dst.value,
      fmt: direct ? "orig" : format.value,
      time: new Date().toLocaleTimeString(),
    };
    history.unshift(h);
    prependHistoryItem(h);
    trimHistory();
    saveHistory();
    text.value = "";
    count.textContent = "0/144";
    count.style.color = "#9ca3af";
    clearInterval(typingTimer);
    typingTimer = 0;
    setTyping(false);
    message.textContent = tv ? L("sentTrans") : L("sent");
    status.textContent = L("sentStatus");
  } catch (e) {
    message.textContent =
      e.message === "missing ai settings"
        ? L("missingAI")
        : e.message === "empty trans"
          ? L("emptyTrans")
          : direct
            ? L("sendFail")
            : trOn.checked
              ? L("transFail")
              : L("sendFail");
    message.className = "m e";
    status.textContent = L("badConn");
  } finally {
    busy = false;
    button.disabled = false;
    trButton.disabled = false;
    text.focus();
  }
}
text.addEventListener("keydown", (e) => {
  if (e.key === "Enter" && !e.shiftKey) {
    e.preventDefault();
    // Enter follows the main action: translate when the translate button is
    // visible, otherwise direct send.
    sendText(trButton.className.indexOf("hide") >= 0);
  }
});
text.addEventListener("input", function () {
  clearTimeout(timer);
  timer = setTimeout(function () {
    sendTyping();
  }, 300);
  if (text.value.trim()) {
    if (!typingTimer) typingTimer = setInterval(sendTyping, 5000);
  } else {
    clearInterval(typingTimer);
    typingTimer = 0;
    setTyping(false);
  }
  var n = text.value.length;
  count.textContent = n + "/144";
  count.style.color = n > 144 ? "#ef4444" : "#9ca3af";
});
button.addEventListener("click", () => sendText(true));
trButton.addEventListener("click", () => sendText(false));
lanBtn.addEventListener("click", enableLan);
exportHistory.addEventListener("click", function () {
  if (!history.length) return;
  let data = history
      .slice()
      .reverse()
      .map(
        (h) => "[" + h.time + "] " + h.text + (h.trans ? "\n" + h.trans : ""),
      )
      .join("\n\n"),
    a = document.createElement("a");
  a.href = URL.createObjectURL(
    new Blob([data], { type: "text/plain;charset=utf-8" }),
  );
  a.download = "vrc-chatbox-history.txt";
  a.click();
  setTimeout(() => URL.revokeObjectURL(a.href), 1000);
  text.focus();
});
clearHistory.addEventListener("click", function () {
  if (!history.length) return;
  if (!confirm(L("clearHistoryConfirm"))) return;
  history = [];
  hlist.innerHTML = "";
  syncHistoryBar();
  saveHistory();
  message.textContent = L("historyCleared");
  message.className = "m";
  text.focus();
});
clearBtn.addEventListener("click", function () {
  text.value = "";
  text.focus();
  count.textContent = "0/144";
  count.style.color = "#9ca3af";
  clearInterval(typingTimer);
  typingTimer = 0;
  setTyping(false);
});
clearBubble.addEventListener("click", async function () {
  if (busy) return;
  busy = true;
  button.disabled = true;
  trButton.disabled = true;
  message.textContent = "清除中...";
  message.className = "m";
  try {
    var r = await fetch("/send", {
      method: "POST",
      headers: { "Content-Type": "text/plain;charset=utf-8" },
      body: "",
    });
    if (!r.ok) throw Error();
    message.textContent = "已清除气泡。";
    status.textContent = L("sentStatus");
  } catch (e) {
    message.textContent = "清除失败。";
    message.className = "m e";
    status.textContent = L("badConn");
  } finally {
    busy = false;
    button.disabled = false;
    trButton.disabled = false;
    text.focus();
  }
});
function saveHistory() {
  try {
    localStorage.setItem(historyKey, JSON.stringify(history));
    fetch("/history", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify(history),
      keepalive: true,
    }).catch(() => {});
  } catch (e) {}
}
async function loadHistory() {
  getHistoryLimit();
  let data = null;
  try {
    let r = await fetch("/history");
    if (r.ok) data = await r.json();
  } catch (e) {}
  try {
    history = (
      (data && data.length
        ? data
        : JSON.parse(localStorage.getItem(historyKey) || "[]")) || []
    )
      .slice(0, getHistoryLimit())
      .map(function (h) {
        h.hid = ++hidCounter;
        return h;
      });
    renderHistory();
  } catch (e) {
    history = [];
  }
}
function syncHistoryBar() {
  var on = history.length;
  hbar.className = on ? "hbar" : "hbar hide";
  hnote.className = on ? "hnote" : "hnote hide";
}
function renderHistory() {
  hlist.innerHTML = history
    .map(function (h) {
      return buildItemHTML(h);
    })
    .join("");
  syncHistoryBar();
}
function findByHid(hid) {
  for (var i = 0; i < history.length; i++) if (history[i].hid === hid) return i;
  return -1;
}
function esc(s) {
  return (s || "")
    .replace(/&/g, "&amp;")
    .replace(/</g, "&lt;")
    .replace(/>/g, "&gt;")
    .replace(/\"/g, "&quot;");
}
function buildItemHTML(h) {
  var escText = esc(h.text),
    escTrans = esc(h.trans),
    t = esc(h.time);
  return (
    '<div class="hitem' +
    (h.trans ? "" : " no-trans") +
    '" id="hitem-' +
    h.hid +
    '">' +
    '<span class="htext"><span class="hsrc" role="button" tabindex="0" ' +
    'onkeydown="cellKey(event,' +
    h.hid +
    ",'src')" +
    '" onpointerdown="cellDown(event,' +
    h.hid +
    ",'src')" +
    '" onpointerup="cellUp(event,' +
    h.hid +
    ",'src')" +
    '" onpointercancel="cellCancel()" onpointerleave="cellCancel()">' +
    escText +
    "</span>" +
    (h.trans
      ? '<span class="htrans" role="button" tabindex="0" onkeydown="cellKey(event,' +
        h.hid +
        ",'trans')" +
        '" onpointerdown="cellDown(event,' +
        h.hid +
        ",'trans')" +
        '" onpointerup="cellUp(event,' +
        h.hid +
        ",'trans')" +
        '" onpointercancel="cellCancel()" onpointerleave="cellCancel()">' +
        escTrans +
        "</span>"
      : "") +
    "</span>" +
    '<span class="htime">' +
    t +
    "</span>" +
    '<span class="del" role="button" tabindex="0" aria-label="' +
    esc(L("deleteHistory")) +
    '" onkeydown="delKey(event,' +
    h.hid +
    ')" onpointerdown="event.stopPropagation()" onpointerup="event.stopPropagation()" onclick="event.stopPropagation();delHistory(' +
    h.hid +
    ')">&times;</span></div>'
  );
}
function prependHistoryItem(h) {
  hlist.insertAdjacentHTML("afterbegin", buildItemHTML(h));
  syncHistoryBar();
}
function trimHistory() {
  let maxHistory = getHistoryLimit();
  while (history.length > maxHistory) {
    var old = history.pop(),
      el = byId("hitem-" + old.hid);
    if (el) el.remove();
  }
  syncHistoryBar();
}
/* History cell gestures.
   Each item splits into source/translation columns. A column:
   - tap       -> fill only that column's text
   - doubletap -> fill original + translation
   - longpress -> send only that column's text
   - hold >1.2s-> send original + translation
   This is fully decoupled from the [翻译格式] setting. */
function cellKey(e, hid, part) {
  if (e.key !== "Enter" && e.key !== " ") return;
  e.preventDefault();
  e.stopPropagation();
  const both = e.shiftKey ? null : part;
  if (e.ctrlKey || e.metaKey) resendCell(hid, both);
  else fillCell(hid, both);
}
function delKey(e, hid) {
  if (e.key !== "Enter" && e.key !== " ") return;
  e.preventDefault();
  e.stopPropagation();
  delHistory(hid);
}
function cellDown(e, hid, part) {
  e.preventDefault();
  e.stopPropagation();
  clearTimeout(tapTimer);
  clearTimeout(pressBothTimer);
  pressHid = hid;
  pressPart = part;
  pressStart = Date.now();
  pressEl = e.currentTarget;
  var item = pressEl.closest(".hitem");
  if (item) item.classList.remove("press-both");
  pressEl.classList.remove("pressing");
  void pressEl.offsetWidth;
  pressEl.classList.add("pressing");
  // At 600 ms the pressed column is full -> start the neighbour column's
  // progress bar (both = 1.2 s total).
  pressBothTimer = setTimeout(function () {
    if (item) item.classList.add("press-both");
  }, 600);
}
function cellUp(e, hid, part) {
  e.preventDefault();
  e.stopPropagation();
  clearTimeout(pressBothTimer);
  if (pressEl && pressHid === hid && pressPart === part) {
    pressEl.classList.remove("pressing");
    var item = pressEl.closest(".hitem");
    if (item) item.classList.remove("press-both");
    pressEl = null;
  } else {
    return;
  }
  var dt = Date.now() - pressStart;
  if (dt < 600) {
    // Tap or double-tap (within 250 ms of the previous tap on any column).
    if (lastTapHid === hid && Date.now() - lastTapTime < 250) {
      clearTimeout(tapTimer);
      fillCell(hid, null); // both
      lastTapHid = -1;
    } else {
      lastTapHid = hid;
      lastTapTime = Date.now();
      tapTimer = setTimeout(function () {
        fillCell(hid, part);
        lastTapHid = -1;
      }, 250);
    }
  } else if (dt < 1200) {
    lastTapHid = -1;
    resendCell(hid, part); // single column
  } else {
    lastTapHid = -1;
    resendCell(hid, null); // both
  }
}
function cellCancel() {
  clearTimeout(tapTimer);
  clearTimeout(pressBothTimer);
  if (pressEl) {
    pressEl.classList.remove("pressing");
    var item = pressEl.closest(".hitem");
    if (item) item.classList.remove("press-both");
    pressEl = null;
  }
}
function cellContent(h, part) {
  return part === "trans" && h.trans ? h.trans : h.text;
}
function fillCell(hid, part) {
  var i = findByHid(hid);
  if (i < 0) return;
  var h = history[i],
    out = part === null ? cellContentBoth(h) : cellContent(h, part);
  text.value = out;
  text.focus();
  // Fill counts as typing: kick the typing indicator immediately so the
  // session/typing state matches the restored content.
  clearTimeout(timer);
  timer = setTimeout(sendTyping, 50);
  if (text.value.trim()) {
    if (!typingTimer) typingTimer = setInterval(sendTyping, 5000);
  }
  var n = out.length;
  count.textContent = n + "/144";
  count.style.color = n > 144 ? "#ef4444" : "#9ca3af";
  message.textContent = L("historyResendReady");
  message.className = "m";
}
function cellContentBoth(h) {
  return h.text + (h.trans ? "\n" + h.trans : "");
}
async function resendCell(hid, part) {
  if (busy) return;
  var i = findByHid(hid);
  if (i < 0) return;
  var h = history[i],
    out = part === null ? cellContentBoth(h) : cellContent(h, part);
  busy = true;
  button.disabled = true;
  trButton.disabled = true;
  message.textContent = L("resending");
  message.className = "m";
  try {
    var r = await fetch("/send", {
      method: "POST",
      headers: { "Content-Type": "text/plain;charset=utf-8" },
      body: out,
    });
    if (!r.ok) throw Error();
    message.textContent = L("resent");
    status.textContent = L("resentStatus");
  } catch (e) {
    message.textContent = L("resendFail");
    message.className = "m e";
    status.textContent = L("badConn");
  } finally {
    busy = false;
    button.disabled = false;
    trButton.disabled = false;
    text.focus();
  }
}
function delHistory(hid) {
  var i = findByHid(hid);
  if (i >= 0) history.splice(i, 1);
  var el = byId("hitem-" + hid);
  if (el) el.remove();
  syncHistoryBar();
  saveHistory();
}
syncStartup();
syncSettingsFromQuick();
loadHistory();
applyLang();
loadPrompts();
load();
refreshLan();

/* Theme toggle — vanilla implementation (no framework needed). */
const THEME_KEY = "vrcChatboxTheme";
function currentTheme() {
  let t = "";
  try { t = localStorage.getItem(THEME_KEY) || ""; } catch (e) {}
  if (t === "dark" || t === "light") return t;
  return window.matchMedia &&
    window.matchMedia("(prefers-color-scheme: dark)").matches
    ? "dark"
    : "light";
}
function applyTheme(t) {
  document.documentElement.setAttribute("data-theme", t);
}
function toggleTheme() {
  const next = currentTheme() === "dark" ? "light" : "dark";
  try { localStorage.setItem(THEME_KEY, next); } catch (e) {}
  applyTheme(next);
  renderThemeIcon(next);
}
function renderThemeIcon(t) {
  const btn = document.getElementById("themeBtn");
  if (btn) btn.textContent = t === "dark" ? "☀️" : "🌙";
}
applyTheme(currentTheme());
renderThemeIcon(currentTheme());
document.getElementById("themeBtn").addEventListener("click", toggleTheme);

/* Clicking blank space (not a control) returns focus to the input box so
   the user can keep typing without an extra click. */
document.addEventListener("click", function (e) {
  var t = e.target;
  if (t === text) return; // already focused
  if (t.closest(".hitem") || t.closest("button") || t.closest("select") ||
      t.closest("input") || t.closest("textarea") || t.closest(".modal")) {
    return; // handled elsewhere / interactive element keeps its focus
  }
  text.focus();
});

/* Expose the handlers referenced by inline HTML in buildItemHTML().
   They must live on window because they are called from generated strings. */
window.cellDown = cellDown;
window.cellUp = cellUp;
window.cellCancel = cellCancel;
window.cellKey = cellKey;
window.delKey = delKey;
window.delHistory = delHistory;
})();
