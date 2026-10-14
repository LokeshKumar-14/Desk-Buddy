// AUTO-GENERATED from web_src/index.html by tools/make_web_page.py - do not edit by hand.
#pragma once
#include <Arduino.h>
static const char SB_WEB_PAGE[] PROGMEM = R"SBHTML(<!doctype html>
<html lang="en"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,viewport-fit=cover">
<title>Smart Buddy</title>
<style>
:root{--bg1:#0b1020;--bg2:#1b1646;--card:rgba(255,255,255,.08);--bd:rgba(255,255,255,.18);--tx:#f2f5fb;--dim:#a9b4c6;--ac:#4fa3ff;--ok:#4ade80;--bad:#ff6b6b;--warn:#ffb020}
@media(prefers-color-scheme:light){:root{--bg1:#dce7ff;--bg2:#f6e6ff;--card:rgba(255,255,255,.6);--bd:rgba(20,32,51,.15);--tx:#142033;--dim:#4b5a70;--ac:#1e6fe0;--ok:#1e9e55;--bad:#d64545}}
*{box-sizing:border-box;-webkit-tap-highlight-color:transparent}
body{margin:0;min-height:100vh;font:16px/1.4 system-ui,-apple-system,Segoe UI,Roboto,sans-serif;color:var(--tx);background:linear-gradient(135deg,var(--bg1),var(--bg2)) fixed;padding-bottom:84px}
header{position:sticky;top:0;z-index:5;display:flex;align-items:center;gap:10px;padding:12px 16px;background:var(--card);backdrop-filter:blur(14px);-webkit-backdrop-filter:blur(14px);border-bottom:1px solid var(--bd)}
header h1{font-size:18px;margin:0;flex:1}
.chip{font-size:12px;padding:3px 9px;border-radius:99px;border:1px solid var(--bd)}
.ok{color:var(--ok)}.bad{color:var(--bad)}.warn{color:var(--warn)}.dim{color:var(--dim)}
main{max-width:640px;margin:0 auto;padding:12px}
section{display:none}section.on{display:block}
.card{background:var(--card);border:1px solid var(--bd);border-radius:18px;padding:14px;margin-bottom:12px;backdrop-filter:blur(14px);-webkit-backdrop-filter:blur(14px)}
.card h2{margin:0 0 8px;font-size:16px}
.big{font-size:40px;font-weight:600;letter-spacing:1px}
.row{display:flex;gap:8px;align-items:center;flex-wrap:wrap}
.row>.grow{flex:1;min-width:0}
input,select,textarea{width:100%;padding:11px 12px;border-radius:12px;border:1px solid var(--bd);background:rgba(0,0,0,.18);color:var(--tx);font:inherit}
input[type=range]{padding:0}
button{padding:10px 14px;border-radius:12px;border:1px solid var(--bd);background:var(--card);color:var(--tx);font:inherit;cursor:pointer}
button:active{transform:translateY(1px);background:var(--ac)}
button.p{background:var(--ac);border-color:transparent;color:#fff}
button.d{background:var(--bad);border-color:transparent;color:#fff}
button:disabled{opacity:.5}
.item{display:flex;align-items:center;gap:10px;padding:10px 0;border-bottom:1px solid var(--bd)}
.item:last-child{border-bottom:0}
.item .t{flex:1;word-break:break-word}
.done .t{text-decoration:line-through;color:var(--dim)}
.chk{width:34px;height:34px;border-radius:50%;padding:0}
nav{position:fixed;bottom:0;left:0;right:0;display:flex;justify-content:space-around;padding:8px 4px calc(8px + env(safe-area-inset-bottom));background:var(--card);backdrop-filter:blur(16px);-webkit-backdrop-filter:blur(16px);border-top:1px solid var(--bd)}
nav button{flex:1;border:0;background:none;font-size:11px;padding:6px 2px;color:var(--dim)}
nav button.on{color:var(--ac)}
nav button b{display:block;font-size:20px;font-weight:400}
.bar{height:8px;border-radius:4px;background:var(--bd);overflow:hidden;margin-top:8px}.bar i{display:block;height:100%;width:0;background:var(--ac);transition:width .2s}
#toast{position:fixed;left:50%;bottom:92px;transform:translateX(-50%);background:var(--card);border:1px solid var(--bd);padding:10px 16px;border-radius:14px;display:none;z-index:9;backdrop-filter:blur(14px);-webkit-backdrop-filter:blur(14px)}
#dlg{position:fixed;inset:0;background:rgba(0,0,0,.6);display:none;align-items:center;justify-content:center;z-index:10;padding:16px}
#dlg>div{background:var(--bg1);border:1px solid var(--bd);border-radius:18px;padding:16px;max-width:380px;width:100%}
#dlg img{width:100%;border-radius:12px;margin-bottom:10px}
.kv{display:grid;grid-template-columns:auto 1fr;gap:4px 12px}.kv span:nth-child(odd){color:var(--dim)}
.pre{width:100%;border-radius:10px;margin-top:8px}
</style></head><body>
<header><h1>Smart Buddy</h1><span class="chip" id="cWifi">wifi</span><span class="chip" id="cSd">sd</span></header>
<main>
<section id="home" class="on">
<div class="card"><div class="big" id="hTime">--:--</div><div class="dim" id="hDate"></div></div>
<div class="card"><h2>Weather</h2><div id="hWx" class="dim">...</div></div>
<div class="card"><h2>To-Do</h2><div id="hTasks" class="dim"></div></div>
<div class="card"><h2>Timer</h2><div class="big" id="hTimer">--</div></div>
<div class="card"><h2>Quick actions</h2><div class="row"><button onclick="post('/api/bg/next',{},'Next wallpaper')">Next wallpaper</button><button onclick="post('/api/weather/refresh',{},'Refreshing')">Refresh weather</button></div></div>
</section>
<section id="todo">
<div class="card"><div class="row"><div class="grow"><input id="tIn" maxlength="80" placeholder="New task..." onkeydown="if(event.key==='Enter')addTask()"></div><button class="p" onclick="addTask()">Add</button></div><div class="dim" id="tCnt" style="margin-top:6px"></div></div>
<div class="card"><h2>Active</h2><div id="tAct"></div></div>
<div class="card"><h2>Completed <span class="dim" style="font-weight:400;font-size:12px">(removed 12 h after completion)</span></h2><div id="tDone"></div></div>
</section>
<section id="wx">
<div class="card"><h2>Current weather</h2><div id="wNow"></div><div class="row" style="margin-top:8px"><button onclick="post('/api/weather/refresh',{},'Refreshing')">Refresh now</button></div></div>
<div class="card"><h2>Location</h2><div class="row"><div class="grow"><input id="wCity" maxlength="47" placeholder="City, e.g. Jaipur"></div><button class="p" onclick="saveCity()">Save</button></div><div class="dim" id="wPlace" style="margin-top:6px"></div></div>
</section>
<section id="pics">
<div class="card"><h2>Add image</h2><input type="file" id="fileA" accept="image/*" onchange="pick(this)"><div class="bar"><i id="pgA"></i></div></div>
<div class="card"><h2>Gallery photos</h2><div id="lGal"></div></div>
</section>
<section id="walls">
<div class="card"><h2>Add image</h2><input type="file" id="fileB" accept="image/*" onchange="pick(this)"><div class="bar"><i id="pgB"></i></div></div>
<div class="card"><h2>Wallpapers</h2><div class="row" style="margin-bottom:6px"><button onclick="post('/api/bg/next',{},'Next wallpaper')">Next</button></div><div id="lBg"></div></div>
</section>
<section id="daily">
<div class="card"><h2>Daily tasks</h2><div class="dim">These reset automatically the next day. Add one main task and optional subtasks below.</div></div>
<div class="card"><input id="dMain" maxlength="56" placeholder="Main daily task"><textarea id="dSubs" rows="3" maxlength="480" placeholder="Subtasks, one per line"></textarea><button class="p" onclick="addDaily()">Add daily task</button></div>
<div class="card"><div id="dList"></div></div>
</section>
<section id="timer">
<div class="card"><div class="big" id="tmBig">--:--</div><div class="dim" id="tmSt"></div><div class="bar"><i id="tmBar"></i></div>
<div class="row" style="margin-top:12px"><button class="p" onclick="tcmd('start')">Start / Resume</button><button onclick="tcmd('pause')">Pause</button><button onclick="tcmd('reset')">Reset</button></div></div>
<div class="card"><h2>Set duration</h2><div class="row"><div class="grow"><input id="tmM" type="number" min="0" max="360" placeholder="min"></div><div class="grow"><input id="tmS" type="number" min="0" max="59" placeholder="sec"></div><button class="p" onclick="setDur()">Set</button></div>
<div class="row" style="margin-top:10px"><button onclick="preset(1500)">Focus 25</button><button onclick="preset(300)">Short 5</button><button onclick="preset(900)">Long 15</button><button onclick="preset(600)">10</button><button onclick="preset(2700)">45</button></div></div>
</section>
<section id="set">
<div class="card"><h2>Settings</h2>
<label class="dim">Theme</label><select id="sTheme"><option value="0">Dark Glass</option><option value="1">Light Glass</option></select>
<label class="dim">Brightness</label><input id="sBr" type="range" min="5" max="100"><div class="dim" id="sBrV"></div>
<label class="dim">Background color</label><input id="sBgC" type="color"><button onclick="useColorBg()">Use color background</button>
<label class="dim">Time zone</label><select id="sTz"></select><input id="sTzC" placeholder="or custom POSIX TZ, e.g. IST-5:30" style="margin-top:6px">
<label class="dim">Slideshow interval (seconds)</label><input id="sSs" type="number" min="3" max="300">
<label class="dim">Default timer (minutes)</label><input id="sTm" type="number" min="1" max="360">
<div class="row" style="margin-top:12px"><button class="p" onclick="saveSet()">Save settings</button></div></div>
<div class="card"><h2>Device status</h2><div class="kv" id="sKv"></div></div>
<div class="card"><button class="d" onclick="reboot()">Reboot device</button></div>
</section>
</main>
<div id="toast"></div>
<div id="dlg"><div><img id="dImg" alt=""><div id="dTxt" style="margin-bottom:12px"></div><div class="row" style="flex-direction:column;align-items:stretch"><button class="p" onclick="up('bg')">Set as Background</button><button onclick="up('gal')">Add to Gallery</button><button onclick="closeDlg()">Cancel</button></div></div></div>
<nav id="nav"></nav>
<script>
const TABS=[['home','&#8962;','Home'],['todo','&#9745;','To-Do'],['daily','&#9776;','Daily'],['wx','&#9728;','Weather'],['pics','&#9638;','Photos'],['walls','&#9635;','Walls'],['timer','&#9201;','Timer'],['set','&#9881;','Settings']];
let S=null,cur='home',tzLoaded=false,formInit=false,pend=null,syncAt=0;
const $=id=>document.getElementById(id);
function esc(s){const d=document.createElement('div');d.textContent=s;return d.innerHTML}
function toast(m,bad){const t=$('toast');t.textContent=m;t.style.color=bad?'var(--bad)':'';t.style.display='block';clearTimeout(t._t);t._t=setTimeout(()=>t.style.display='none',2600)}
async function post(u,o,ok){try{const r=await fetch(u,{method:'POST',body:new URLSearchParams(o||{})});const j=await r.json();if(r.ok){if(ok)toast(j.msg&&ok===true?j.msg:ok)}else toast(j.msg||'Error',1);poll();return r.ok}catch(e){toast('Device not reachable',1);return false}}
$('nav').innerHTML=TABS.map(t=>`<button data-t="${t[0]}" onclick="show('${t[0]}')"><b>${t[1]}</b>${t[2]}</button>`).join('');
function show(t){cur=t;document.querySelectorAll('section').forEach(s=>s.classList.toggle('on',s.id===t));document.querySelectorAll('nav button').forEach(b=>b.classList.toggle('on',b.dataset.t===t));if(t==='pics')lists();if(t==='walls')lists();render()}
async function poll(){try{const r=await fetch('/api/state',{cache:'no-store'});S=await r.json();syncAt=performance.now();render()}catch(e){$('cWifi').textContent='offline';$('cWifi').className='chip bad'}}
function fmt(ms){let s=Math.ceil(ms/1000),h=Math.floor(s/3600),m=Math.floor(s/60)%60;s%=60;const p=n=>String(n).padStart(2,'0');return(h?h+':':'')+p(m)+':'+p(s)}
function remNow(){if(!S)return 0;const t=S.timer;return t.state==='running'?Math.max(0,t.remaining-(performance.now()-syncAt)):t.remaining}
function wxLine(w){if(!w.valid){return {no_location:'Set a city on the Weather tab',waiting:'Waiting for data...',fetching:'Updating...',error:w.err||'Update failed',ok:'No data'}[w.status]||'No data'}
let u=w.updated?new Date(w.updated*1000).toLocaleTimeString([],{hour:'2-digit',minute:'2-digit'}):'--';
return `<b>${Math.round(w.temp)}&deg;C</b> ${esc(w.desc)}${w.hum>=0?' &middot; humidity '+w.hum+'%':''}<br><span class="dim">${esc(w.place)} &middot; updated ${u}</span>`+(w.stale?' <span class="warn">STALE</span>':'')+(!S.wifi.internet?' <span class="warn">OFFLINE</span>':'')}
function render(){if(!S)return;
const w=S.wifi;$('cWifi').textContent=w.connected?(w.internet?'online':'no internet'):'no wifi';$('cWifi').className='chip '+(w.connected&&w.internet?'ok':'warn');
$('cSd').textContent=S.sd.ok?'SD ok':'no SD';$('cSd').className='chip '+(S.sd.ok?'ok':'bad');
if(cur==='home'){$('hTime').textContent=S.time.valid?S.time.str.slice(11,16):'--:--';$('hDate').textContent=S.time.valid?S.time.str.slice(0,10):'Clock not synced yet';
$('hWx').innerHTML=wxLine(S.weather);const a=S.tasks.filter(t=>!t.done).length;$('hTasks').textContent=a+' active, '+(S.tasks.length-a)+' completed';$('hTimer').textContent=fmt(remNow())}
if(cur==='todo'){const mk=t=>`<div class="item ${t.done?'done':''}"><button class="chk ${t.done?'p':''}" onclick="post('/api/task/toggle',{id:${t.id}})">${t.done?'&#10003;':''}</button><div class="t">${esc(t.text)}</div><button onclick="post('/api/task/delete',{id:${t.id}})">&#10005;</button></div>`;
const A=S.tasks.filter(t=>!t.done),D=S.tasks.filter(t=>t.done);$('tAct').innerHTML=A.map(mk).join('')||'<div class="dim">Nothing to do</div>';$('tDone').innerHTML=D.map(mk).join('')||'<div class="dim">No completed tasks</div>';$('tCnt').textContent=S.tasks.length+' / '+S.maxTasks+' tasks'+(S.sd.ok?'':' - SD card missing, not saved')}
if(cur==='daily'){const mkSub=(t,s)=>`<div class="item ${s.done?'done':''}" style="padding-left:20px"><button class="chk ${s.done?'p':''}" onclick="post('/api/daily/sub/toggle',{id:${t.id},sid:${s.id}})">${s.done?'&#10003;':''}</button><div class="t">${esc(s.text)}</div><button onclick="post('/api/daily/sub/delete',{id:${t.id},sid:${s.id}})">&#10005;</button></div>`;
const mk=t=>`<div class="item ${t.done?'done':''}"><button class="chk ${t.done?'p':''}" onclick="post('/api/daily/toggle',{id:${t.id}})">${t.done?'&#10003;':''}</button><div class="t"><b>${esc(t.text)}</b>${t.subs.map(s=>mkSub(t,s)).join('')}<div class="row"><input id="sub${t.id}" maxlength="56" placeholder="Add subtask"><button onclick="addDailySub(${t.id})">Add sub</button></div></div><button onclick="post('/api/daily/delete',{id:${t.id}})">&#10005;</button></div>`;
$('dList').innerHTML=(S.daily||[]).map(mk).join('')||'<div class="dim">No daily tasks yet</div>'}
if(cur==='wx'){$('wNow').innerHTML=wxLine(S.weather);$('wPlace').textContent=S.settings.place?('Using: '+S.settings.place):(S.settings.city?'Looking up '+S.settings.city+'...':'No location set');if(document.activeElement!==$('wCity')&&!$('wCity').value)$('wCity').value=S.settings.city}
if(cur==='timer'||cur==='home'){const t=S.timer,r=remNow();$('tmBig').textContent=fmt(r);$('tmSt').textContent={idle:'Ready',running:'Running',paused:'Paused',done:'Finished'}[t.state]+' - default '+fmt(t.duration*1000);$('tmBar').style.width=(t.state==='idle'?0:100*(1-r/(t.duration*1000)))+'%'}
if(cur==='set'){if(!formInit){formInit=true;$('sTheme').value=S.settings.theme;$('sSs').value=S.settings.slideshow;$('sTm').value=Math.round(S.settings.timer/60);$('sBr').value=S.settings.brightness;$('sBgC').value='#'+S.settings.bgColor;loadTz()}
$('sBrV').textContent=$('sBr').value+'%';
const k=[['Wi-Fi',w.connected?w.ssid:'not connected'],['IP address',w.ip||'-'],['Signal',w.connected?w.rssi+' dBm':'-'],['Internet',w.internet?'yes':'no'],['Clock',S.time.valid?S.time.str:'not synced'],['SD card',S.sd.ok?S.sd.totalMB+' MB, '+S.sd.freeMB+' MB free':'missing'],['Free heap',Math.round(S.heap/1024)+' KB'],['Free PSRAM',Math.round(S.psram/1024)+' KB'],['Uptime',Math.floor(S.uptime/3600)+'h '+Math.floor(S.uptime/60)%60+'m']];
$('sKv').innerHTML=k.map(x=>'<span>'+x[0]+'</span><span>'+esc(String(x[1]))+'</span>').join('')}}
async function loadTz(){if(tzLoaded)return;const l=await (await fetch('/api/tz')).json();tzLoaded=true;$('sTz').innerHTML=l.map(x=>`<option value="${x.tz}">${esc(x.label)}</option>`).join('')+'<option value="">Custom (below)</option>';const i=l.findIndex(x=>x.tz===S.settings.tz);$('sTz').value=i>=0?l[i].tz:'';$('sTzC').value=i>=0?'':S.settings.tz}
async function addTask(){const v=$('tIn').value.trim();if(!v)return;if(await post('/api/task/add',{text:v},'Task added'))$('tIn').value=''}
async function addDaily(){const v=$('dMain').value.trim();if(!v)return;if(await post('/api/daily/add',{text:v,subs:$('dSubs').value},'Daily task added')){$('dMain').value='';$('dSubs').value=''}}
async function addDailySub(id){const i=$('sub'+id),v=i.value.trim();if(!v)return;if(await post('/api/daily/sub/add',{id,text:v},'Subtask added'))i.value=''}
function saveCity(){post('/api/weather/city',{city:$('wCity').value.trim()},true)}
function tcmd(c){post('/api/timer',{cmd:c})}
function setDur(){const s=(+$('tmM').value||0)*60+(+$('tmS').value||0);post('/api/timer',{cmd:'set',sec:s},'Duration set')}
function preset(s){post('/api/timer',{cmd:'set',sec:s},'Duration set')}
function saveSet(){const tz=$('sTzC').value.trim()||$('sTz').value;const o={theme:$('sTheme').value,slideshow:$('sSs').value,timer:(+$('sTm').value||25)*60,brightness:$('sBr').value,bgColor:$('sBgC').value};if(tz)o.tz=tz;post('/api/settings',o,'Settings saved')}
function useColorBg(){post('/api/settings',{brightness:$('sBr').value,bgColor:$('sBgC').value,bgColorOnly:1},'Color background saved')}
function reboot(){if(confirm('Reboot Smart Buddy now?'))post('/api/reboot',{},'Rebooting...')}
async function lists(){for(const [k,id] of [['gal','lGal'],['bg','lBg']]){try{const l=await (await fetch('/api/files?kind='+k,{cache:'no-store'})).json();
$(id).innerHTML=l.map((f,i)=>`<div class="item"><div class="t">${esc(f.name)}<div class="dim" style="font-size:12px">${Math.round(f.size/1024)} KB${f.active?' &middot; <span class="ok">active</span>':''}</div><img class="pre" style="display:none" id="pv${k}${i}"></div><div class="row">${k==='bg'?`<button class="p" onclick='sel(${JSON.stringify(f.name)})'>Use</button>`:''}<button onclick='prev("${k}",${JSON.stringify(f.name)},"pv${k}${i}")'>View</button><button class="d" onclick='del("${k}",${JSON.stringify(f.name)})'>&#10005;</button></div></div>`).join('')||'<div class="dim">Empty</div>'}catch(e){}}}
function prev(k,n,id){const i=$(id);if(i.style.display==='none'){i.src='/api/file?kind='+k+'&name='+encodeURIComponent(n);i.style.display='block'}else{i.style.display='none';i.removeAttribute('src')}}
async function sel(n){if(await post('/api/bg/select',{name:n},'Applying wallpaper...'))setTimeout(lists,2500)}
async function del(k,n){if(!confirm('Delete '+n+'?'))return;if(await post('/api/file/delete',{kind:k,name:n},'Deleted'))lists()}
function pick(inp){const f=inp.files[0];if(!f)return;pend=f;$('dImg').src=URL.createObjectURL(f);$('dTxt').textContent=f.name+' - where should it go?';$('dlg').style.display='flex'}
function closeDlg(){$('dlg').style.display='none';$('fileA').value='';$('fileB').value='';pend=null}
function toJpeg(file,kind){return new Promise((res,rej)=>{const im=new Image();im.onload=()=>{let w=im.naturalWidth,h=im.naturalHeight,c=document.createElement('canvas'),x=c.getContext('2d'),sx=0,sy=0,sw=w,sh=h;
if(kind==='bg'){c.width=800;c.height=480;if(w*480>h*800){sw=Math.round(h*800/480);sx=Math.round((w-sw)/2)}else{sh=Math.round(w*480/800);sy=Math.round((h-sh)/2)}x.drawImage(im,sx,sy,sw,sh,0,0,800,480)}
else{const m=Math.min(1,1280/w,800/h);c.width=Math.round(w*m);c.height=Math.round(h*m);x.drawImage(im,0,0,c.width,c.height)}
c.toBlob(b=>b?res(b):rej(new Error('encode')),'image/jpeg',.86)};im.onerror=()=>rej(new Error('decode'));im.src=URL.createObjectURL(file)})}
async function up(kind){const f=pend;if(!f)return;$('dlg').style.display='none';const pg=$(kind==='bg'?'pgB':'pgA');
try{const b=await toJpeg(f,kind);const name=f.name.replace(/\.[^.]*$/,'')+'.jpg';const fd=new FormData();fd.append('file',b,name);
const x=new XMLHttpRequest();x.open('POST','/api/upload?kind='+kind+(kind==='bg'?'&select=1':''));x.upload.onprogress=e=>{if(e.lengthComputable)pg.style.width=(100*e.loaded/e.total)+'%'};
x.onload=()=>{pg.style.width='0';let j={};try{j=JSON.parse(x.responseText)}catch(e){}if(x.status===200){toast(kind==='bg'?'Saved as wallpaper':'Added to gallery');lists();poll()}else toast(j.msg||'Upload failed',1)};
x.onerror=()=>{pg.style.width='0';toast('Upload failed - device unreachable',1)};x.send(fd)}catch(e){toast('Could not read that image',1)}closeDlg()}
show('home');poll();setInterval(poll,2500);setInterval(()=>{if(cur==='timer'||cur==='home')render()},500);
</script></body></html>)SBHTML";
