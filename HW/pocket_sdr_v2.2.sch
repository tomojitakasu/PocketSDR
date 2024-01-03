<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE eagle SYSTEM "eagle.dtd">
<eagle version="9.6.2">
<drawing>
<settings>
<setting alwaysvectorfont="no"/>
<setting verticaltext="up"/>
</settings>
<grid distance="0.1" unitdist="inch" unit="inch" style="lines" multiple="1" display="no" altdistance="0.01" altunitdist="inch" altunit="inch"/>
<layers>
<layer number="1" name="Top" color="4" fill="1" visible="no" active="no"/>
<layer number="16" name="Bottom" color="1" fill="1" visible="no" active="no"/>
<layer number="17" name="Pads" color="2" fill="1" visible="no" active="no"/>
<layer number="18" name="Vias" color="2" fill="1" visible="no" active="no"/>
<layer number="19" name="Unrouted" color="6" fill="1" visible="no" active="no"/>
<layer number="20" name="Dimension" color="24" fill="1" visible="no" active="no"/>
<layer number="21" name="tPlace" color="7" fill="1" visible="no" active="no"/>
<layer number="22" name="bPlace" color="7" fill="1" visible="no" active="no"/>
<layer number="23" name="tOrigins" color="15" fill="1" visible="no" active="no"/>
<layer number="24" name="bOrigins" color="15" fill="1" visible="no" active="no"/>
<layer number="25" name="tNames" color="7" fill="1" visible="no" active="no"/>
<layer number="26" name="bNames" color="7" fill="1" visible="no" active="no"/>
<layer number="27" name="tValues" color="7" fill="1" visible="no" active="no"/>
<layer number="28" name="bValues" color="7" fill="1" visible="no" active="no"/>
<layer number="29" name="tStop" color="7" fill="3" visible="no" active="no"/>
<layer number="30" name="bStop" color="7" fill="6" visible="no" active="no"/>
<layer number="31" name="tCream" color="7" fill="4" visible="no" active="no"/>
<layer number="32" name="bCream" color="7" fill="5" visible="no" active="no"/>
<layer number="33" name="tFinish" color="6" fill="3" visible="no" active="no"/>
<layer number="34" name="bFinish" color="6" fill="6" visible="no" active="no"/>
<layer number="35" name="tGlue" color="7" fill="4" visible="no" active="no"/>
<layer number="36" name="bGlue" color="7" fill="5" visible="no" active="no"/>
<layer number="37" name="tTest" color="7" fill="1" visible="no" active="no"/>
<layer number="38" name="bTest" color="7" fill="1" visible="no" active="no"/>
<layer number="39" name="tKeepout" color="4" fill="11" visible="no" active="no"/>
<layer number="40" name="bKeepout" color="1" fill="11" visible="no" active="no"/>
<layer number="41" name="tRestrict" color="4" fill="10" visible="no" active="no"/>
<layer number="42" name="bRestrict" color="1" fill="10" visible="no" active="no"/>
<layer number="43" name="vRestrict" color="2" fill="10" visible="no" active="no"/>
<layer number="44" name="Drills" color="7" fill="1" visible="no" active="no"/>
<layer number="45" name="Holes" color="7" fill="1" visible="no" active="no"/>
<layer number="46" name="Milling" color="3" fill="1" visible="no" active="no"/>
<layer number="47" name="Measures" color="7" fill="1" visible="no" active="no"/>
<layer number="48" name="Document" color="7" fill="1" visible="no" active="no"/>
<layer number="49" name="Reference" color="7" fill="1" visible="no" active="no"/>
<layer number="51" name="tDocu" color="7" fill="1" visible="no" active="no"/>
<layer number="52" name="bDocu" color="7" fill="1" visible="no" active="no"/>
<layer number="88" name="SimResults" color="9" fill="1" visible="yes" active="yes"/>
<layer number="89" name="SimProbes" color="9" fill="1" visible="yes" active="yes"/>
<layer number="90" name="Modules" color="5" fill="1" visible="yes" active="yes"/>
<layer number="91" name="Nets" color="2" fill="1" visible="yes" active="yes"/>
<layer number="92" name="Busses" color="1" fill="1" visible="yes" active="yes"/>
<layer number="93" name="Pins" color="2" fill="1" visible="no" active="yes"/>
<layer number="94" name="Symbols" color="4" fill="1" visible="yes" active="yes"/>
<layer number="95" name="Names" color="7" fill="1" visible="yes" active="yes"/>
<layer number="96" name="Values" color="7" fill="1" visible="yes" active="yes"/>
<layer number="97" name="Info" color="7" fill="1" visible="yes" active="yes"/>
<layer number="98" name="Guide" color="6" fill="1" visible="yes" active="yes"/>
</layers>
<schematic xreflabel="%F%N/%S.%C%R" xrefpart="/%S.%C%R">
<libraries>
<library name="ublox">
<packages>
<package name="C1608">
<description>&lt;b&gt;CAPACITOR&lt;/b&gt;</description>
<wire x1="-0.8" y1="0.4" x2="0.8" y2="0.4" width="0.1016" layer="51"/>
<wire x1="-0.8" y1="-0.4" x2="0.8" y2="-0.4" width="0.1016" layer="51"/>
<smd name="1" x="-0.8" y="0" dx="0.8" dy="0.8" layer="1"/>
<smd name="2" x="0.8" y="0" dx="0.8" dy="0.8" layer="1"/>
<text x="-1.73" y="1" size="0.8128" layer="25" font="vector" ratio="12">&gt;NAME</text>
<wire x1="-0.8" y1="-0.4" x2="-0.8" y2="0.4" width="0.1016" layer="51"/>
<wire x1="0.8" y1="-0.4" x2="0.8" y2="0.4" width="0.1016" layer="51"/>
<wire x1="-1.3" y1="0.5" x2="-1.3" y2="-0.5" width="0.1016" layer="21"/>
<wire x1="-1.27" y1="0.508" x2="1.3" y2="0.5" width="0.1016" layer="21"/>
<wire x1="1.3" y1="0.5" x2="1.3" y2="-0.5" width="0.1016" layer="21"/>
<wire x1="1.3" y1="-0.5" x2="-1.27" y2="-0.508" width="0.1016" layer="21"/>
</package>
</packages>
<symbols>
<symbol name="INDACTOR">
<text x="-4.826" y="2.032" size="1.27" layer="95">&gt;NAME</text>
<text x="0" y="2.032" size="1.27" layer="96">&gt;VALUE</text>
<pin name="P$1" x="-2.54" y="0" visible="off" length="point" direction="pas"/>
<pin name="P$2" x="2.54" y="0" visible="off" length="point" direction="pas"/>
<wire x1="-2.54" y1="0" x2="-2.54" y2="0.762" width="0.254" layer="94"/>
<wire x1="-2.54" y1="0.762" x2="-1.27" y2="0.762" width="0.254" layer="94" curve="-180"/>
<wire x1="-1.27" y1="0" x2="-1.27" y2="0.762" width="0.254" layer="94"/>
<wire x1="-1.27" y1="0.762" x2="0" y2="0.762" width="0.254" layer="94" curve="-180"/>
<wire x1="0" y1="0" x2="0" y2="0.762" width="0.254" layer="94"/>
<wire x1="0" y1="0.762" x2="1.27" y2="0.762" width="0.254" layer="94" curve="-180"/>
<wire x1="1.27" y1="0" x2="1.27" y2="0.762" width="0.254" layer="94"/>
<wire x1="1.27" y1="0.762" x2="2.54" y2="0.762" width="0.254" layer="94" curve="-180"/>
<wire x1="2.54" y1="0.762" x2="2.54" y2="0" width="0.254" layer="94"/>
</symbol>
</symbols>
<devicesets>
<deviceset name="L1608">
<gates>
<gate name="G$1" symbol="INDACTOR" x="0" y="0"/>
</gates>
<devices>
<device name="" package="C1608">
<connects>
<connect gate="G$1" pin="P$1" pad="1"/>
<connect gate="G$1" pin="P$2" pad="2"/>
</connects>
<technologies>
<technology name=""/>
</technologies>
</device>
</devices>
</deviceset>
</devicesets>
</library>
<library name="power">
<packages>
</packages>
<symbols>
<symbol name="+3.3V">
<wire x1="-2.54" y1="0" x2="2.54" y2="0" width="0.254" layer="94"/>
<circle x="0" y="0" radius="0.254" width="0.254" layer="94"/>
<circle x="0" y="0" radius="0.508" width="0.254" layer="94"/>
<text x="-2.54" y="1.27" size="1.778" layer="96">&gt;VALUE</text>
<pin name="+3.3V" x="0" y="-2.54" visible="off" length="short" direction="sup" rot="R90"/>
</symbol>
<symbol name="GND">
<wire x1="-2.54" y1="-2.54" x2="-1.27" y2="-2.54" width="0.4064" layer="94"/>
<wire x1="0.635" y1="-2.54" x2="2.54" y2="-2.54" width="0.4064" layer="94"/>
<wire x1="-1.27" y1="-2.54" x2="-2.54" y2="-4.445" width="0.254" layer="94"/>
<wire x1="2.54" y1="-2.54" x2="1.27" y2="-4.445" width="0.254" layer="94"/>
<wire x1="0.635" y1="-2.54" x2="-1.27" y2="-2.54" width="0.4064" layer="94"/>
<wire x1="0.635" y1="-2.54" x2="-0.635" y2="-4.445" width="0.254" layer="94"/>
<pin name="GND" x="0" y="0" visible="off" length="short" direction="sup" rot="R270"/>
</symbol>
</symbols>
<devicesets>
<deviceset name="+3.3V">
<gates>
<gate name="G$1" symbol="+3.3V" x="0" y="0"/>
</gates>
<devices>
<device name="">
<technologies>
<technology name=""/>
</technologies>
</device>
</devices>
</deviceset>
<deviceset name="GND">
<description>GND</description>
<gates>
<gate name="GND" symbol="GND" x="0" y="0"/>
</gates>
<devices>
<device name="">
<technologies>
<technology name=""/>
</technologies>
</device>
</devices>
</deviceset>
</devicesets>
</library>
<library name="ttaka_ic">
<packages>
<package name="QFN56">
<smd name="P1" x="-3.25" y="-4" dx="0.8" dy="0.25" layer="1" rot="R90" thermals="no"/>
<wire x1="-3.5" y1="4" x2="-4" y2="4" width="0.1016" layer="21"/>
<wire x1="-4" y1="4" x2="-4" y2="3.5" width="0.1016" layer="21"/>
<wire x1="-4" y1="-3.5" x2="-4" y2="-4" width="0.1016" layer="21"/>
<wire x1="-3.5" y1="-4" x2="-4" y2="-4" width="0.1016" layer="21"/>
<wire x1="4" y1="-3.5" x2="4" y2="-4" width="0.1016" layer="21"/>
<wire x1="3.5" y1="-4" x2="4" y2="-4" width="0.1016" layer="21"/>
<wire x1="3.5" y1="4" x2="4" y2="4" width="0.1016" layer="21"/>
<wire x1="4" y1="3.5" x2="4" y2="4" width="0.1016" layer="21"/>
<circle x="-3.1" y="-3.1" radius="0.2" width="0.1016" layer="21"/>
<smd name="EP1" x="0" y="0" dx="5.2" dy="4.5" layer="1" thermals="no" cream="no"/>
<wire x1="-4" y1="4" x2="-4" y2="-4" width="0.1016" layer="51"/>
<wire x1="4" y1="-4" x2="4" y2="4" width="0.1016" layer="51"/>
<wire x1="4" y1="4" x2="-4" y2="4" width="0.1016" layer="51"/>
<smd name="EP2" x="-1.35" y="1.175" dx="2.4" dy="2.05" layer="1" stop="no" thermals="no"/>
<smd name="P2" x="-2.75" y="-4" dx="0.8" dy="0.25" layer="1" rot="R90" thermals="no"/>
<wire x1="-4" y1="-4" x2="4" y2="-4" width="0.1016" layer="51"/>
<smd name="P3" x="-2.25" y="-4" dx="0.8" dy="0.25" layer="1" rot="R90" thermals="no"/>
<smd name="P4" x="-1.75" y="-4" dx="0.8" dy="0.25" layer="1" rot="R90" thermals="no"/>
<smd name="P5" x="-1.25" y="-4" dx="0.8" dy="0.25" layer="1" rot="R90" thermals="no"/>
<smd name="P6" x="-0.75" y="-4" dx="0.8" dy="0.25" layer="1" rot="R90" thermals="no"/>
<smd name="P7" x="-0.25" y="-4" dx="0.8" dy="0.25" layer="1" rot="R90" thermals="no"/>
<smd name="P8" x="0.25" y="-4" dx="0.8" dy="0.25" layer="1" rot="R90" thermals="no"/>
<smd name="P9" x="0.75" y="-4" dx="0.8" dy="0.25" layer="1" rot="R90" thermals="no"/>
<smd name="P10" x="1.25" y="-4" dx="0.8" dy="0.25" layer="1" rot="R90" thermals="no"/>
<smd name="P11" x="1.75" y="-4" dx="0.8" dy="0.25" layer="1" rot="R90" thermals="no"/>
<smd name="P12" x="2.25" y="-4" dx="0.8" dy="0.25" layer="1" rot="R90" thermals="no"/>
<smd name="P13" x="2.75" y="-4" dx="0.8" dy="0.25" layer="1" rot="R90" thermals="no"/>
<smd name="P14" x="3.25" y="-4" dx="0.8" dy="0.25" layer="1" rot="R90" thermals="no"/>
<smd name="P15" x="4" y="-3.25" dx="0.8" dy="0.25" layer="1" rot="R180" thermals="no"/>
<smd name="P16" x="4" y="-2.75" dx="0.8" dy="0.25" layer="1" rot="R180" thermals="no"/>
<smd name="P17" x="4" y="-2.25" dx="0.8" dy="0.25" layer="1" rot="R180" thermals="no"/>
<smd name="P18" x="4" y="-1.75" dx="0.8" dy="0.25" layer="1" rot="R180" thermals="no"/>
<smd name="P19" x="4" y="-1.25" dx="0.8" dy="0.25" layer="1" rot="R180" thermals="no"/>
<smd name="P20" x="4" y="-0.75" dx="0.8" dy="0.25" layer="1" rot="R180" thermals="no"/>
<smd name="P21" x="4" y="-0.25" dx="0.8" dy="0.25" layer="1" rot="R180" thermals="no"/>
<smd name="P22" x="4" y="0.25" dx="0.8" dy="0.25" layer="1" rot="R180" thermals="no"/>
<smd name="P23" x="4" y="0.75" dx="0.8" dy="0.25" layer="1" rot="R180" thermals="no"/>
<smd name="P24" x="4" y="1.25" dx="0.8" dy="0.25" layer="1" rot="R180" thermals="no"/>
<smd name="P25" x="4" y="1.75" dx="0.8" dy="0.25" layer="1" rot="R180" thermals="no"/>
<smd name="P26" x="4" y="2.25" dx="0.8" dy="0.25" layer="1" rot="R180" thermals="no"/>
<smd name="P27" x="4" y="2.75" dx="0.8" dy="0.25" layer="1" rot="R180" thermals="no"/>
<smd name="P28" x="4" y="3.25" dx="0.8" dy="0.25" layer="1" rot="R180" thermals="no"/>
<smd name="P29" x="3.25" y="4" dx="0.8" dy="0.25" layer="1" rot="R270" thermals="no"/>
<smd name="P30" x="2.75" y="4" dx="0.8" dy="0.25" layer="1" rot="R270" thermals="no"/>
<smd name="P31" x="2.25" y="4" dx="0.8" dy="0.25" layer="1" rot="R270" thermals="no"/>
<smd name="P32" x="1.75" y="4" dx="0.8" dy="0.25" layer="1" rot="R270" thermals="no"/>
<smd name="P33" x="1.25" y="4" dx="0.8" dy="0.25" layer="1" rot="R270" thermals="no"/>
<smd name="P34" x="0.75" y="4" dx="0.8" dy="0.25" layer="1" rot="R270" thermals="no"/>
<smd name="P35" x="0.25" y="4" dx="0.8" dy="0.25" layer="1" rot="R270" thermals="no"/>
<smd name="P36" x="-0.25" y="4" dx="0.8" dy="0.25" layer="1" rot="R270" thermals="no"/>
<smd name="P37" x="-0.75" y="4" dx="0.8" dy="0.25" layer="1" rot="R270" thermals="no"/>
<smd name="P38" x="-1.25" y="4" dx="0.8" dy="0.25" layer="1" rot="R270" thermals="no"/>
<smd name="P39" x="-1.75" y="4" dx="0.8" dy="0.25" layer="1" rot="R270" thermals="no"/>
<smd name="P40" x="-2.25" y="4" dx="0.8" dy="0.25" layer="1" rot="R270" thermals="no"/>
<smd name="P41" x="-2.75" y="4" dx="0.8" dy="0.25" layer="1" rot="R270" thermals="no"/>
<smd name="P42" x="-3.25" y="4" dx="0.8" dy="0.25" layer="1" rot="R270" thermals="no"/>
<smd name="P43" x="-4" y="3.25" dx="0.8" dy="0.25" layer="1" thermals="no"/>
<smd name="P44" x="-4" y="2.75" dx="0.8" dy="0.25" layer="1" thermals="no"/>
<smd name="P45" x="-4" y="2.25" dx="0.8" dy="0.25" layer="1" thermals="no"/>
<smd name="P46" x="-4" y="1.75" dx="0.8" dy="0.25" layer="1" thermals="no"/>
<smd name="P47" x="-4" y="1.25" dx="0.8" dy="0.25" layer="1" thermals="no"/>
<smd name="P48" x="-4" y="0.75" dx="0.8" dy="0.25" layer="1" thermals="no"/>
<smd name="P49" x="-4" y="0.25" dx="0.8" dy="0.25" layer="1" thermals="no"/>
<smd name="P50" x="-4" y="-0.25" dx="0.8" dy="0.25" layer="1" thermals="no"/>
<smd name="P51" x="-4" y="-0.75" dx="0.8" dy="0.25" layer="1" thermals="no"/>
<smd name="P52" x="-4" y="-1.25" dx="0.8" dy="0.25" layer="1" thermals="no"/>
<smd name="P53" x="-4" y="-1.75" dx="0.8" dy="0.25" layer="1" thermals="no"/>
<smd name="P54" x="-4" y="-2.25" dx="0.8" dy="0.25" layer="1" thermals="no"/>
<smd name="P55" x="-4" y="-2.75" dx="0.8" dy="0.25" layer="1" thermals="no"/>
<smd name="P56" x="-4" y="-3.25" dx="0.8" dy="0.25" layer="1" thermals="no"/>
<smd name="EP3" x="1.35" y="1.175" dx="2.4" dy="2.05" layer="1" stop="no" thermals="no"/>
<smd name="EP4" x="-1.35" y="-1.175" dx="2.4" dy="2.05" layer="1" stop="no" thermals="no"/>
<smd name="EP5" x="1.35" y="-1.175" dx="2.4" dy="2.05" layer="1" stop="no" thermals="no"/>
<pad name="P$3" x="0" y="1.8" drill="0.35" diameter="0.7" stop="no" thermals="no"/>
<pad name="P$1" x="-1.1" y="1.8" drill="0.35" diameter="0.7" stop="no" thermals="no"/>
<pad name="P$2" x="1.1" y="1.8" drill="0.35" diameter="0.7" stop="no" thermals="no"/>
<pad name="P$4" x="-2.2" y="1.8" drill="0.35" diameter="0.7" stop="no" thermals="no"/>
<pad name="P$5" x="2.2" y="1.8" drill="0.35" diameter="0.7" stop="no" thermals="no"/>
<pad name="P$6" x="-2.2" y="0.6" drill="0.35" diameter="0.7" stop="no" thermals="no"/>
<pad name="P$7" x="-1.1" y="0.6" drill="0.35" diameter="0.7" stop="no" thermals="no"/>
<pad name="P$8" x="0" y="0.6" drill="0.35" diameter="0.7" stop="no" thermals="no"/>
<pad name="P$9" x="1.1" y="0.6" drill="0.35" diameter="0.7" stop="no" thermals="no"/>
<pad name="P$10" x="2.2" y="0.6" drill="0.35" diameter="0.7" stop="no" thermals="no"/>
<pad name="P$11" x="-2.2" y="-0.6" drill="0.35" diameter="0.7" stop="no" thermals="no"/>
<pad name="P$12" x="-1.1" y="-0.6" drill="0.35" diameter="0.7" stop="no" thermals="no"/>
<pad name="P$13" x="0" y="-0.6" drill="0.35" diameter="0.7" stop="no" thermals="no"/>
<pad name="P$14" x="1.1" y="-0.6" drill="0.35" diameter="0.7" stop="no" thermals="no"/>
<pad name="P$15" x="2.2" y="-0.6" drill="0.35" diameter="0.7" stop="no" thermals="no"/>
<pad name="P$16" x="-2.2" y="-1.8" drill="0.35" diameter="0.7" stop="no" thermals="no"/>
<pad name="P$17" x="-1.1" y="-1.8" drill="0.35" diameter="0.7" stop="no" thermals="no"/>
<pad name="P$18" x="0" y="-1.8" drill="0.35" diameter="0.7" stop="no" thermals="no"/>
<pad name="P$19" x="1.1" y="-1.8" drill="0.35" diameter="0.7" stop="no" thermals="no"/>
<pad name="P$20" x="2.2" y="-1.8" drill="0.35" diameter="0.7" stop="no" thermals="no"/>
</package>
<package name="TSSOP_8P">
<wire x1="1.5" y1="2.2" x2="1.5" y2="-2.2" width="0.1016" layer="21"/>
<wire x1="-1.5" y1="-2.2" x2="-1.5" y2="2.2" width="0.1016" layer="21"/>
<circle x="-1" y="-1.7" radius="0.2236" width="0.1016" layer="21"/>
<smd name="8" x="-0.975" y="2.95" dx="1.45" dy="0.4" layer="1" rot="R90"/>
<text x="-2.2" y="4.3" size="0.8128" layer="21" font="vector" ratio="12">&gt;NAME</text>
<smd name="7" x="-0.325" y="2.95" dx="1.45" dy="0.4" layer="1" rot="R90"/>
<smd name="6" x="0.325" y="2.95" dx="1.45" dy="0.4" layer="1" rot="R90"/>
<smd name="5" x="0.975" y="2.95" dx="1.45" dy="0.4" layer="1" rot="R90"/>
<smd name="1" x="-0.975" y="-2.95" dx="1.45" dy="0.4" layer="1" rot="R90"/>
<smd name="2" x="-0.325" y="-2.95" dx="1.45" dy="0.4" layer="1" rot="R90"/>
<smd name="3" x="0.325" y="-2.95" dx="1.45" dy="0.4" layer="1" rot="R90"/>
<smd name="4" x="0.975" y="-2.95" dx="1.45" dy="0.4" layer="1" rot="R90"/>
<wire x1="-1.5" y1="2.2" x2="-1.5" y2="-2.2" width="0.1016" layer="51"/>
<wire x1="-1.5" y1="-2.2" x2="1.5" y2="-2.2" width="0.1016" layer="51"/>
<wire x1="1.5" y1="-2.2" x2="1.5" y2="2.2" width="0.1016" layer="51"/>
<wire x1="-1.5" y1="2.2" x2="-1.3" y2="2.2" width="0.1016" layer="21"/>
<wire x1="1.5" y1="2.2" x2="1.3" y2="2.2" width="0.1016" layer="21"/>
<wire x1="1.5" y1="-2.2" x2="1.3" y2="-2.2" width="0.1016" layer="21"/>
<wire x1="-1.5" y1="-2.2" x2="-1.3" y2="-2.2" width="0.1016" layer="21"/>
<wire x1="-1.5" y1="2.2" x2="1.5" y2="2.2" width="0.1016" layer="51"/>
<wire x1="-1.5" y1="-2.2" x2="1.5" y2="-2.2" width="0.1016" layer="51"/>
</package>
<package name="TQFN28">
<smd name="11" x="2.34" y="0" dx="0.95" dy="0.26" layer="1"/>
<smd name="12" x="2.34" y="0.5" dx="0.95" dy="0.26" layer="1"/>
<smd name="13" x="2.34" y="1" dx="0.95" dy="0.26" layer="1"/>
<smd name="14" x="2.34" y="1.5" dx="0.95" dy="0.26" layer="1"/>
<smd name="10" x="2.34" y="-0.5" dx="0.95" dy="0.26" layer="1"/>
<smd name="9" x="2.34" y="-1" dx="0.95" dy="0.26" layer="1"/>
<smd name="8" x="2.34" y="-1.5" dx="0.95" dy="0.26" layer="1"/>
<smd name="22" x="-2.34" y="1.5" dx="0.95" dy="0.26" layer="1"/>
<smd name="23" x="-2.34" y="1" dx="0.95" dy="0.26" layer="1"/>
<smd name="24" x="-2.34" y="0.5" dx="0.95" dy="0.26" layer="1"/>
<smd name="25" x="-2.34" y="0" dx="0.95" dy="0.26" layer="1"/>
<smd name="26" x="-2.34" y="-0.5" dx="0.95" dy="0.26" layer="1"/>
<smd name="27" x="-2.34" y="-1" dx="0.95" dy="0.26" layer="1"/>
<smd name="28" x="-2.34" y="-1.5" dx="0.95" dy="0.26" layer="1"/>
<smd name="18" x="0" y="2.34" dx="0.95" dy="0.26" layer="1" rot="R90"/>
<smd name="19" x="-0.5" y="2.34" dx="0.95" dy="0.26" layer="1" rot="R90"/>
<smd name="20" x="-1" y="2.34" dx="0.95" dy="0.26" layer="1" rot="R90"/>
<smd name="21" x="-1.5" y="2.34" dx="0.95" dy="0.26" layer="1" rot="R90"/>
<smd name="17" x="0.5" y="2.34" dx="0.95" dy="0.26" layer="1" rot="R90"/>
<smd name="16" x="1" y="2.34" dx="0.95" dy="0.26" layer="1" rot="R90"/>
<smd name="15" x="1.5" y="2.34" dx="0.95" dy="0.26" layer="1" rot="R90"/>
<smd name="4" x="0" y="-2.34" dx="0.95" dy="0.26" layer="1" rot="R90"/>
<smd name="3" x="-0.5" y="-2.34" dx="0.95" dy="0.26" layer="1" rot="R90"/>
<smd name="2" x="-1" y="-2.34" dx="0.95" dy="0.26" layer="1" rot="R90"/>
<smd name="1" x="-1.5" y="-2.34" dx="0.95" dy="0.26" layer="1" rot="R90"/>
<smd name="5" x="0.5" y="-2.34" dx="0.95" dy="0.26" layer="1" rot="R90"/>
<smd name="6" x="1" y="-2.34" dx="0.95" dy="0.26" layer="1" rot="R90"/>
<smd name="7" x="1.5" y="-2.34" dx="0.95" dy="0.26" layer="1" rot="R90"/>
<smd name="EP" x="0" y="0" dx="3.25" dy="3.25" layer="1" thermals="no" cream="no"/>
<wire x1="-1.8" y1="2.5" x2="-2.5" y2="2.5" width="0.1016" layer="21"/>
<wire x1="-2.5" y1="2.5" x2="-2.5" y2="1.8" width="0.1016" layer="21"/>
<wire x1="-2.5" y1="-1.8" x2="-2.5" y2="-2.5" width="0.1016" layer="21"/>
<wire x1="-1.8" y1="-2.5" x2="-2.5" y2="-2.5" width="0.1016" layer="21"/>
<wire x1="2.5" y1="-1.8" x2="2.5" y2="-2.5" width="0.1016" layer="21"/>
<wire x1="1.8" y1="-2.5" x2="2.5" y2="-2.5" width="0.1016" layer="21"/>
<wire x1="1.8" y1="2.5" x2="2.5" y2="2.5" width="0.1016" layer="21"/>
<wire x1="2.5" y1="1.8" x2="2.5" y2="2.5" width="0.1016" layer="21"/>
<circle x="-2.1" y="-2.1" radius="0.2" width="0.1016" layer="21"/>
<smd name="C3" x="-0.85" y="0.85" dx="1.4" dy="1.4" layer="1" rot="R180" stop="no" thermals="no"/>
<wire x1="-2.5" y1="2.5" x2="-2.5" y2="-2.5" width="0.1016" layer="51"/>
<wire x1="-2.5" y1="-2.5" x2="2.5" y2="-2.5" width="0.1016" layer="51"/>
<wire x1="2.5" y1="-2.5" x2="2.5" y2="2.5" width="0.1016" layer="51"/>
<wire x1="2.5" y1="2.5" x2="-2.5" y2="2.5" width="0.1016" layer="51"/>
<smd name="C1" x="-0.85" y="-0.85" dx="1.4" dy="1.4" layer="1" stop="no" thermals="no"/>
<smd name="C2" x="0.85" y="-0.85" dx="1.4" dy="1.4" layer="1" stop="no" thermals="no"/>
<smd name="C4" x="0.85" y="0.85" dx="1.4" dy="1.4" layer="1" rot="R180" stop="no" thermals="no"/>
<pad name="V7" x="-1.1" y="1.1" drill="0.35" stop="no" thermals="no"/>
<pad name="V5" x="0" y="0" drill="0.35" stop="no" thermals="no"/>
<pad name="V8" x="0" y="1.1" drill="0.35" stop="no" thermals="no"/>
<pad name="V9" x="1.1" y="1.1" drill="0.35" stop="no" thermals="no"/>
<pad name="V4" x="-1.1" y="0" drill="0.35" stop="no" thermals="no"/>
<pad name="V6" x="1.1" y="0" drill="0.35" stop="no" thermals="no"/>
<pad name="V3" x="1.1" y="-1.1" drill="0.35" stop="no" thermals="no"/>
<pad name="V2" x="0" y="-1.1" drill="0.35" stop="no" thermals="no"/>
<pad name="V1" x="-1.1" y="-1.1" drill="0.35" stop="no" thermals="no"/>
<text x="-4.699" y="3.175" size="0.8128" layer="21" font="vector" ratio="12">&gt;NAME</text>
<text x="0.381" y="3.175" size="0.8128" layer="21" font="vector" ratio="12">&gt;VALUE</text>
</package>
</packages>
<symbols>
<symbol name="CY7C6813A">
<wire x1="-35.56" y1="66.04" x2="-35.56" y2="-60.96" width="0.254" layer="94"/>
<wire x1="-35.56" y1="-60.96" x2="35.56" y2="-60.96" width="0.254" layer="94"/>
<wire x1="35.56" y1="-60.96" x2="35.56" y2="66.04" width="0.254" layer="94"/>
<wire x1="35.56" y1="66.04" x2="-35.56" y2="66.04" width="0.254" layer="94"/>
<text x="-5.08" y="6.35" size="1.778" layer="95" ratio="20">&gt;NAME</text>
<text x="3.81" y="-6.35" size="1.778" layer="96" ratio="20" rot="R180">&gt;VALUE</text>
<pin name="CTL0/FLAGA" x="-38.1" y="30.48" length="short"/>
<pin name="CTL1/FLAGB" x="-38.1" y="25.4" length="short"/>
<pin name="CLKOUT/PE1" x="-38.1" y="5.08" length="short"/>
<pin name="RDY0/SLRD" x="-38.1" y="0" length="short"/>
<pin name="XTALOUT" x="-38.1" y="-10.16" length="short" direction="out"/>
<pin name="XTALIN" x="-38.1" y="-15.24" length="short" direction="in"/>
<pin name="PA3/WU2" x="38.1" y="45.72" length="short" rot="R180"/>
<pin name="PA4/FIFOADR0" x="38.1" y="40.64" length="short" rot="R180"/>
<pin name="PA5/FIFOADR1" x="38.1" y="35.56" length="short" rot="R180"/>
<pin name="PB0/FD0" x="38.1" y="20.32" length="short" rot="R180"/>
<pin name="PB4/FD4" x="38.1" y="0" length="short" rot="R180"/>
<pin name="PB5/FD5" x="38.1" y="-5.08" length="short" rot="R180"/>
<pin name="PB6/FD6" x="38.1" y="-10.16" length="short" rot="R180"/>
<pin name="PB7/FD7" x="38.1" y="-15.24" length="short" rot="R180"/>
<pin name="PA2/SLOE" x="38.1" y="50.8" length="short" rot="R180"/>
<pin name="PB3/FD3" x="38.1" y="5.08" length="short" rot="R180"/>
<pin name="PB2/FD2" x="38.1" y="10.16" length="short" rot="R180"/>
<pin name="PB1/FD1" x="38.1" y="15.24" length="short" rot="R180"/>
<pin name="PA1/INT1#" x="38.1" y="55.88" length="short" rot="R180"/>
<pin name="PA0/INT0#" x="38.1" y="60.96" length="short" rot="R180"/>
<pin name="PA7/FLAGD/SLCS#" x="38.1" y="25.4" length="short" rot="R180"/>
<pin name="PA6/PKTEND" x="38.1" y="30.48" length="short" rot="R180"/>
<pin name="GND@2" x="-12.7" y="-63.5" length="short" direction="sup" rot="R90"/>
<pin name="SDA" x="-38.1" y="35.56" length="short" direction="out"/>
<pin name="SCL" x="-38.1" y="40.64" length="short" direction="out"/>
<pin name="VCC@1" x="-17.78" y="68.58" length="short" direction="sup" rot="R270"/>
<pin name="VCC@2" x="-12.7" y="68.58" length="short" direction="sup" rot="R270"/>
<pin name="CTL2/FLAGC" x="-38.1" y="20.32" length="short"/>
<pin name="RESET#" x="-38.1" y="15.24" length="short" direction="in"/>
<pin name="WAKEUP" x="-38.1" y="10.16" length="short" direction="in"/>
<pin name="RDY1/SLWR" x="-38.1" y="-5.08" length="short"/>
<pin name="GND@1" x="-17.78" y="-63.5" length="short" direction="sup" rot="R90"/>
<pin name="PD0/FD8" x="38.1" y="-20.32" length="short" rot="R180"/>
<pin name="PD1/FD9" x="38.1" y="-25.4" length="short" rot="R180"/>
<pin name="PD2/FD10" x="38.1" y="-30.48" length="short" rot="R180"/>
<pin name="PD3/FD11" x="38.1" y="-35.56" length="short" rot="R180"/>
<pin name="PD4/FD12" x="38.1" y="-40.64" length="short" rot="R180"/>
<pin name="PD5/FD13" x="38.1" y="-45.72" length="short" rot="R180"/>
<pin name="PD6/FD14" x="38.1" y="-50.8" length="short" rot="R180"/>
<pin name="PD7/FD15" x="38.1" y="-55.88" length="short" rot="R180"/>
<pin name="VCC@3" x="-7.62" y="68.58" length="short" direction="sup" rot="R270"/>
<pin name="VCC@4" x="-2.54" y="68.58" length="short" direction="sup" rot="R270"/>
<pin name="VCC@5" x="2.54" y="68.58" length="short" direction="sup" rot="R270"/>
<pin name="VCC@6" x="7.62" y="68.58" length="short" direction="sup" rot="R270"/>
<pin name="AVCC@1" x="12.7" y="68.58" length="short" direction="sup" rot="R270"/>
<pin name="AVCC@2" x="17.78" y="68.58" length="short" direction="sup" rot="R270"/>
<pin name="GND@3" x="-7.62" y="-63.5" length="short" direction="sup" rot="R90"/>
<pin name="GND@4" x="-2.54" y="-63.5" length="short" direction="sup" rot="R90"/>
<pin name="GND@5" x="2.54" y="-63.5" length="short" direction="sup" rot="R90"/>
<pin name="GND@6" x="7.62" y="-63.5" length="short" direction="sup" rot="R90"/>
<pin name="AGND@1" x="12.7" y="-63.5" length="short" direction="sup" rot="R90"/>
<pin name="AGND@2" x="17.78" y="-63.5" length="short" direction="sup" rot="R90"/>
<pin name="DPLUS" x="-38.1" y="-20.32" length="short"/>
<pin name="DMINUS" x="-38.1" y="-25.4" length="short"/>
<pin name="IFCLK/PE0" x="-38.1" y="-30.48" length="short" direction="in"/>
<pin name="RESERVED" x="-38.1" y="-35.56" length="short"/>
<pin name="EP" x="-38.1" y="-50.8" length="short" direction="sup"/>
</symbol>
<symbol name="AT24C256C">
<wire x1="-10.16" y1="10.16" x2="-10.16" y2="-10.16" width="0.254" layer="94"/>
<wire x1="-10.16" y1="-10.16" x2="10.16" y2="-10.16" width="0.254" layer="94"/>
<wire x1="10.16" y1="-10.16" x2="10.16" y2="10.16" width="0.254" layer="94"/>
<wire x1="10.16" y1="10.16" x2="-10.16" y2="10.16" width="0.254" layer="94"/>
<text x="-7.62" y="13.97" size="1.778" layer="95">&gt;NAME</text>
<text x="-7.62" y="-15.24" size="1.778" layer="96">&gt;VALUE</text>
<pin name="A0" x="-12.7" y="7.62" length="short" direction="in"/>
<pin name="A1" x="-12.7" y="2.54" length="short" direction="in"/>
<pin name="A2" x="-12.7" y="-2.54" length="short" direction="in"/>
<pin name="GND" x="-12.7" y="-7.62" length="short" direction="sup"/>
<pin name="VCC" x="12.7" y="7.62" length="short" direction="sup" rot="R180"/>
<pin name="WP" x="12.7" y="2.54" length="short" direction="in" rot="R180"/>
<pin name="SCL" x="12.7" y="-2.54" length="short" direction="in" rot="R180"/>
<pin name="SDA" x="12.7" y="-7.62" length="short" rot="R180"/>
</symbol>
<symbol name="MAX2771">
<wire x1="-27.94" y1="27.94" x2="-27.94" y2="-27.94" width="0.254" layer="94"/>
<wire x1="-27.94" y1="-27.94" x2="27.94" y2="-27.94" width="0.254" layer="94"/>
<wire x1="27.94" y1="-27.94" x2="27.94" y2="27.94" width="0.254" layer="94"/>
<wire x1="27.94" y1="27.94" x2="-27.94" y2="27.94" width="0.254" layer="94"/>
<pin name="CPOUT" x="-15.24" y="-33.02" length="middle" rot="R90"/>
<pin name="VCCCP" x="-10.16" y="-33.02" length="middle" direction="sup" rot="R90"/>
<pin name="XTAL" x="-5.08" y="-33.02" length="middle" rot="R90"/>
<pin name="CLKOUT" x="0" y="-33.02" length="middle" rot="R90"/>
<pin name="ADC_CLKIN" x="5.08" y="-33.02" length="middle" rot="R90"/>
<pin name="VCCD" x="10.16" y="-33.02" length="middle" direction="sup" rot="R90"/>
<pin name="SDATA" x="15.24" y="-33.02" length="middle" rot="R90"/>
<pin name="SCLK" x="33.02" y="-15.24" length="middle" direction="in" rot="R180"/>
<pin name="CSN" x="33.02" y="-10.16" length="middle" direction="in" rot="R180"/>
<pin name="Q1" x="33.02" y="-5.08" length="middle" direction="out" rot="R180"/>
<pin name="Q0" x="33.02" y="0" length="middle" direction="out" rot="R180"/>
<pin name="I0" x="33.02" y="5.08" length="middle" direction="out" rot="R180"/>
<pin name="I1" x="33.02" y="10.16" length="middle" direction="out" rot="R180"/>
<pin name="VCCADC" x="33.02" y="15.24" length="middle" direction="sup" rot="R180"/>
<pin name="ANAIPOUT" x="15.24" y="33.02" length="middle" rot="R270"/>
<pin name="ANAINOUT" x="10.16" y="33.02" length="middle" rot="R270"/>
<pin name="LD" x="5.08" y="33.02" length="middle" direction="out" rot="R270"/>
<pin name="SHDN" x="0" y="33.02" length="middle" rot="R270"/>
<pin name="LNAIN_HI" x="-5.08" y="33.02" length="middle" rot="R270"/>
<pin name="LNAIN_LO" x="-10.16" y="33.02" length="middle" rot="R270"/>
<pin name="NC" x="-15.24" y="33.02" length="middle" direction="nc" rot="R270"/>
<pin name="VCCRF" x="-33.02" y="15.24" length="middle" direction="sup"/>
<pin name="LNAOUT_LO" x="-33.02" y="10.16" length="middle" direction="out"/>
<pin name="LNAOUT_HI" x="-33.02" y="5.08" length="middle" direction="out"/>
<pin name="MIXIN_HI" x="-33.02" y="0" length="middle" direction="in"/>
<pin name="MIXIN_LO" x="-33.02" y="-5.08" length="middle" direction="in"/>
<pin name="VCCIF" x="-33.02" y="-10.16" length="middle" direction="sup"/>
<pin name="VCCVCO" x="-33.02" y="-15.24" length="middle" direction="sup"/>
<pin name="EP" x="33.02" y="-22.86" length="middle" direction="sup" rot="R180"/>
<text x="-5.08" y="10.16" size="1.27" layer="95">&gt;NAME</text>
<text x="-5.08" y="5.08" size="1.27" layer="96">&gt;VALUE</text>
</symbol>
</symbols>
<devicesets>
<deviceset name="CY7C6813A">
<gates>
<gate name="G$1" symbol="CY7C6813A" x="0" y="0"/>
</gates>
<devices>
<device name="" package="QFN56">
<connects>
<connect gate="G$1" pin="AGND@1" pad="P6"/>
<connect gate="G$1" pin="AGND@2" pad="P10"/>
<connect gate="G$1" pin="AVCC@1" pad="P3"/>
<connect gate="G$1" pin="AVCC@2" pad="P7"/>
<connect gate="G$1" pin="CLKOUT/PE1" pad="P54"/>
<connect gate="G$1" pin="CTL0/FLAGA" pad="P29"/>
<connect gate="G$1" pin="CTL1/FLAGB" pad="P30"/>
<connect gate="G$1" pin="CTL2/FLAGC" pad="P31"/>
<connect gate="G$1" pin="DMINUS" pad="P9"/>
<connect gate="G$1" pin="DPLUS" pad="P8"/>
<connect gate="G$1" pin="EP" pad="EP1 EP2 EP3 EP4 EP5 P$1 P$2 P$3 P$4 P$5 P$6 P$7 P$8 P$9 P$10 P$11 P$12 P$13 P$14 P$15 P$16 P$17 P$18 P$19 P$20"/>
<connect gate="G$1" pin="GND@1" pad="P12"/>
<connect gate="G$1" pin="GND@2" pad="P26"/>
<connect gate="G$1" pin="GND@3" pad="P28"/>
<connect gate="G$1" pin="GND@4" pad="P41"/>
<connect gate="G$1" pin="GND@5" pad="P53"/>
<connect gate="G$1" pin="GND@6" pad="P56"/>
<connect gate="G$1" pin="IFCLK/PE0" pad="P13"/>
<connect gate="G$1" pin="PA0/INT0#" pad="P33"/>
<connect gate="G$1" pin="PA1/INT1#" pad="P34"/>
<connect gate="G$1" pin="PA2/SLOE" pad="P35"/>
<connect gate="G$1" pin="PA3/WU2" pad="P36"/>
<connect gate="G$1" pin="PA4/FIFOADR0" pad="P37"/>
<connect gate="G$1" pin="PA5/FIFOADR1" pad="P38"/>
<connect gate="G$1" pin="PA6/PKTEND" pad="P39"/>
<connect gate="G$1" pin="PA7/FLAGD/SLCS#" pad="P40"/>
<connect gate="G$1" pin="PB0/FD0" pad="P18"/>
<connect gate="G$1" pin="PB1/FD1" pad="P19"/>
<connect gate="G$1" pin="PB2/FD2" pad="P20"/>
<connect gate="G$1" pin="PB3/FD3" pad="P21"/>
<connect gate="G$1" pin="PB4/FD4" pad="P22"/>
<connect gate="G$1" pin="PB5/FD5" pad="P23"/>
<connect gate="G$1" pin="PB6/FD6" pad="P24"/>
<connect gate="G$1" pin="PB7/FD7" pad="P25"/>
<connect gate="G$1" pin="PD0/FD8" pad="P45"/>
<connect gate="G$1" pin="PD1/FD9" pad="P46"/>
<connect gate="G$1" pin="PD2/FD10" pad="P47"/>
<connect gate="G$1" pin="PD3/FD11" pad="P48"/>
<connect gate="G$1" pin="PD4/FD12" pad="P49"/>
<connect gate="G$1" pin="PD5/FD13" pad="P50"/>
<connect gate="G$1" pin="PD6/FD14" pad="P51"/>
<connect gate="G$1" pin="PD7/FD15" pad="P52"/>
<connect gate="G$1" pin="RDY0/SLRD" pad="P1"/>
<connect gate="G$1" pin="RDY1/SLWR" pad="P2"/>
<connect gate="G$1" pin="RESERVED" pad="P14"/>
<connect gate="G$1" pin="RESET#" pad="P42"/>
<connect gate="G$1" pin="SCL" pad="P15"/>
<connect gate="G$1" pin="SDA" pad="P16"/>
<connect gate="G$1" pin="VCC@1" pad="P11"/>
<connect gate="G$1" pin="VCC@2" pad="P17"/>
<connect gate="G$1" pin="VCC@3" pad="P27"/>
<connect gate="G$1" pin="VCC@4" pad="P32"/>
<connect gate="G$1" pin="VCC@5" pad="P55"/>
<connect gate="G$1" pin="VCC@6" pad="P43"/>
<connect gate="G$1" pin="WAKEUP" pad="P44"/>
<connect gate="G$1" pin="XTALIN" pad="P5"/>
<connect gate="G$1" pin="XTALOUT" pad="P4"/>
</connects>
<technologies>
<technology name=""/>
</technologies>
</device>
</devices>
</deviceset>
<deviceset name="AT24C256C">
<gates>
<gate name="G$1" symbol="AT24C256C" x="0" y="0"/>
</gates>
<devices>
<device name="" package="TSSOP_8P">
<connects>
<connect gate="G$1" pin="A0" pad="1"/>
<connect gate="G$1" pin="A1" pad="2"/>
<connect gate="G$1" pin="A2" pad="3"/>
<connect gate="G$1" pin="GND" pad="4"/>
<connect gate="G$1" pin="SCL" pad="6"/>
<connect gate="G$1" pin="SDA" pad="5"/>
<connect gate="G$1" pin="VCC" pad="8"/>
<connect gate="G$1" pin="WP" pad="7"/>
</connects>
<technologies>
<technology name=""/>
</technologies>
</device>
</devices>
</deviceset>
<deviceset name="MAX2771">
<gates>
<gate name="G$1" symbol="MAX2771" x="0" y="0"/>
</gates>
<devices>
<device name="" package="TQFN28">
<connects>
<connect gate="G$1" pin="ADC_CLKIN" pad="5"/>
<connect gate="G$1" pin="ANAINOUT" pad="16"/>
<connect gate="G$1" pin="ANAIPOUT" pad="15"/>
<connect gate="G$1" pin="CLKOUT" pad="4"/>
<connect gate="G$1" pin="CPOUT" pad="1"/>
<connect gate="G$1" pin="CSN" pad="9"/>
<connect gate="G$1" pin="EP" pad="C1 C2 C3 C4 EP V1 V2 V3 V4 V5 V6 V7 V8 V9"/>
<connect gate="G$1" pin="I0" pad="12"/>
<connect gate="G$1" pin="I1" pad="13"/>
<connect gate="G$1" pin="LD" pad="17"/>
<connect gate="G$1" pin="LNAIN_HI" pad="19"/>
<connect gate="G$1" pin="LNAIN_LO" pad="20"/>
<connect gate="G$1" pin="LNAOUT_HI" pad="24"/>
<connect gate="G$1" pin="LNAOUT_LO" pad="23"/>
<connect gate="G$1" pin="MIXIN_HI" pad="25"/>
<connect gate="G$1" pin="MIXIN_LO" pad="26"/>
<connect gate="G$1" pin="NC" pad="21"/>
<connect gate="G$1" pin="Q0" pad="11"/>
<connect gate="G$1" pin="Q1" pad="10"/>
<connect gate="G$1" pin="SCLK" pad="8"/>
<connect gate="G$1" pin="SDATA" pad="7"/>
<connect gate="G$1" pin="SHDN" pad="18"/>
<connect gate="G$1" pin="VCCADC" pad="14"/>
<connect gate="G$1" pin="VCCCP" pad="2"/>
<connect gate="G$1" pin="VCCD" pad="6"/>
<connect gate="G$1" pin="VCCIF" pad="27"/>
<connect gate="G$1" pin="VCCRF" pad="22"/>
<connect gate="G$1" pin="VCCVCO" pad="28"/>
<connect gate="G$1" pin="XTAL" pad="3"/>
</connects>
<technologies>
<technology name=""/>
</technologies>
</device>
</devices>
</deviceset>
</devicesets>
</library>
<library name="ttaka_power">
<packages>
</packages>
<symbols>
<symbol name="GND">
<wire x1="-2.54" y1="-2.54" x2="-1.27" y2="-2.54" width="0.4064" layer="94"/>
<wire x1="0.635" y1="-2.54" x2="2.54" y2="-2.54" width="0.4064" layer="94"/>
<wire x1="-1.27" y1="-2.54" x2="-2.54" y2="-4.445" width="0.254" layer="94"/>
<wire x1="2.54" y1="-2.54" x2="1.27" y2="-4.445" width="0.254" layer="94"/>
<wire x1="0.635" y1="-2.54" x2="-1.27" y2="-2.54" width="0.4064" layer="94"/>
<wire x1="0.635" y1="-2.54" x2="-0.635" y2="-4.445" width="0.254" layer="94"/>
<pin name="GND" x="0" y="0" visible="off" length="short" direction="sup" rot="R270"/>
</symbol>
<symbol name="+3.3V">
<wire x1="-2.54" y1="0" x2="2.54" y2="0" width="0.254" layer="94"/>
<circle x="0" y="0" radius="0.254" width="0.254" layer="94"/>
<circle x="0" y="0" radius="0.508" width="0.254" layer="94"/>
<text x="-2.54" y="1.27" size="1.778" layer="96">&gt;VALUE</text>
<pin name="+3.3V" x="0" y="-2.54" visible="off" length="short" direction="sup" rot="R90"/>
</symbol>
</symbols>
<devicesets>
<deviceset name="GND">
<description>GND</description>
<gates>
<gate name="GND" symbol="GND" x="0" y="0"/>
</gates>
<devices>
<device name="">
<technologies>
<technology name=""/>
</technologies>
</device>
</devices>
</deviceset>
<deviceset name="+3.3V">
<gates>
<gate name="G$1" symbol="+3.3V" x="0" y="0"/>
</gates>
<devices>
<device name="">
<technologies>
<technology name=""/>
</technologies>
</device>
</devices>
</deviceset>
</devicesets>
</library>
<library name="ttaka_misc">
<packages>
<package name="SOT23">
<wire x1="-1.55" y1="0.7" x2="1.55" y2="0.7" width="0.1016" layer="21"/>
<wire x1="1.55" y1="0.7" x2="1.55" y2="-0.7" width="0.1016" layer="21"/>
<wire x1="1.55" y1="-0.7" x2="-1.55" y2="-0.7" width="0.1016" layer="21"/>
<wire x1="-1.55" y1="-0.7" x2="-1.55" y2="0.7" width="0.1016" layer="21"/>
<smd name="5" x="-0.95" y="1.5" dx="1.016" dy="0.508" layer="1" rot="R90" thermals="no"/>
<smd name="4" x="0.95" y="1.5" dx="1.016" dy="0.508" layer="1" rot="R90" thermals="no"/>
<smd name="2" x="0" y="-1.5" dx="1.016" dy="0.508" layer="1" rot="R90" thermals="no"/>
<smd name="3" x="0.95" y="-1.5" dx="1.016" dy="0.508" layer="1" rot="R90" thermals="no"/>
<smd name="1" x="-0.95" y="-1.5" dx="1.016" dy="0.508" layer="1" rot="R90" thermals="no"/>
<text x="-1.8" y="-2.1" size="0.8128" layer="21" font="vector" ratio="12" rot="R90">&gt;NAME</text>
<wire x1="-1.55" y1="0.7" x2="-1.55" y2="-0.7" width="0.1016" layer="51"/>
<wire x1="-1.55" y1="-0.7" x2="1.55" y2="-0.7" width="0.1016" layer="51"/>
<wire x1="1.55" y1="-0.7" x2="1.55" y2="0.7" width="0.1016" layer="51"/>
<wire x1="1.55" y1="0.7" x2="-1.55" y2="0.7" width="0.1016" layer="51"/>
</package>
<package name="RF_DIV">
<wire x1="-1.02" y1="0.645" x2="-1.02" y2="-0.645" width="0.1016" layer="51"/>
<wire x1="-1.02" y1="-0.645" x2="1.02" y2="-0.645" width="0.1016" layer="51"/>
<wire x1="1.02" y1="-0.645" x2="1.02" y2="0.645" width="0.1016" layer="51"/>
<wire x1="1.02" y1="0.645" x2="-1.02" y2="0.645" width="0.1016" layer="51"/>
<smd name="2" x="0" y="0.5" dx="0.41" dy="0.33" layer="1" thermals="no"/>
<smd name="5" x="0" y="-0.5" dx="0.41" dy="0.33" layer="1" thermals="no"/>
<smd name="3" x="-0.66" y="0.5" dx="0.41" dy="0.33" layer="1" thermals="no"/>
<smd name="4" x="-0.66" y="-0.5" dx="0.41" dy="0.33" layer="1" thermals="no"/>
<smd name="6" x="0.66" y="-0.5" dx="0.41" dy="0.33" layer="1" thermals="no"/>
<smd name="1" x="0.66" y="0.5" dx="0.41" dy="0.33" layer="1" thermals="no"/>
<wire x1="-1.02" y1="0.645" x2="-1.02" y2="-0.645" width="0.1016" layer="21"/>
<wire x1="1.02" y1="-0.645" x2="1.02" y2="0.645" width="0.1016" layer="21"/>
<wire x1="-1.02" y1="0.645" x2="-0.95" y2="0.645" width="0.1016" layer="21"/>
<wire x1="1.02" y1="-0.645" x2="0.95" y2="-0.645" width="0.1016" layer="21"/>
<wire x1="-1.02" y1="-0.645" x2="-0.95" y2="-0.645" width="0.1016" layer="21"/>
<wire x1="1.02" y1="0.645" x2="0.95" y2="0.645" width="0.1016" layer="21"/>
<wire x1="-0.36" y1="0.645" x2="-0.29" y2="0.645" width="0.1016" layer="21"/>
<wire x1="0.29" y1="0.645" x2="0.36" y2="0.645" width="0.1016" layer="21"/>
<wire x1="0.36" y1="-0.645" x2="0.29" y2="-0.645" width="0.1016" layer="21"/>
<wire x1="-0.29" y1="-0.645" x2="-0.36" y2="-0.645" width="0.1016" layer="21"/>
<circle x="0.7" y="0" radius="0.1" width="0.1016" layer="21"/>
</package>
<package name="XTAL_SMD">
<smd name="P1" x="-1.1" y="-0.8" dx="1.15" dy="1.4" layer="1" rot="R90" thermals="no"/>
<wire x1="-0.3" y1="1.35" x2="0.3" y2="1.35" width="0.1016" layer="21"/>
<wire x1="-0.3" y1="-1.35" x2="0.3" y2="-1.35" width="0.1016" layer="21"/>
<wire x1="-1.7" y1="-0.15" x2="-1.7" y2="0.15" width="0.1016" layer="21"/>
<wire x1="1.7" y1="-0.15" x2="1.7" y2="0.15" width="0.1016" layer="21"/>
<circle x="-1.92" y="-1.72" radius="0.2" width="0.1016" layer="21"/>
<smd name="P2" x="1.1" y="-0.8" dx="1.15" dy="1.4" layer="1" rot="R90" thermals="no"/>
<smd name="P3" x="1.1" y="0.8" dx="1.15" dy="1.4" layer="1" rot="R90" thermals="no"/>
<smd name="P4" x="-1.1" y="0.8" dx="1.15" dy="1.4" layer="1" rot="R90" thermals="no"/>
<wire x1="-1.7" y1="1.35" x2="-1.7" y2="-1.35" width="0.1016" layer="51"/>
<wire x1="-1.7" y1="-1.35" x2="1.7" y2="-1.35" width="0.1016" layer="51"/>
<wire x1="1.7" y1="-1.35" x2="1.7" y2="1.35" width="0.1016" layer="51"/>
<wire x1="1.7" y1="1.35" x2="-1.7" y2="1.35" width="0.1016" layer="51"/>
<text x="-4.064" y="1.905" size="0.8128" layer="21" font="vector" ratio="12">&gt;NAME</text>
<text x="0.254" y="1.905" size="0.8128" layer="21" font="vector" ratio="12">&gt;VALUE</text>
</package>
<package name="TCXO">
<smd name="1" x="-1.2" y="-0.9" dx="0.9" dy="1" layer="1" rot="R90" thermals="no"/>
<smd name="2" x="1.2" y="-0.9" dx="0.9" dy="1" layer="1" rot="R90" thermals="no"/>
<smd name="4" x="-1.2" y="0.9" dx="0.9" dy="1" layer="1" rot="R90" thermals="no"/>
<smd name="3" x="1.2" y="0.9" dx="0.9" dy="1" layer="1" rot="R90" thermals="no"/>
<wire x1="-0.45" y1="1.25" x2="0.45" y2="1.25" width="0.1016" layer="21"/>
<wire x1="-0.45" y1="-1.25" x2="0.45" y2="-1.25" width="0.1016" layer="21"/>
<wire x1="-1.6" y1="-0.2" x2="-1.6" y2="0.2" width="0.1016" layer="21"/>
<wire x1="1.6" y1="-0.2" x2="1.6" y2="0.2" width="0.1016" layer="21"/>
<circle x="-0.42" y="-0.88" radius="0.2" width="0.1016" layer="21"/>
<wire x1="-1.6" y1="1.26" x2="-1.6" y2="-1.26" width="0.1016" layer="51"/>
<wire x1="-1.6" y1="-1.26" x2="1.6" y2="-1.26" width="0.1016" layer="51"/>
<wire x1="1.6" y1="-1.26" x2="1.6" y2="1.26" width="0.1016" layer="51"/>
<wire x1="1.6" y1="1.26" x2="-1.6" y2="1.26" width="0.1016" layer="51"/>
<text x="-2.286" y="1.905" size="0.8128" layer="21" font="vector" ratio="12">&gt;NAME</text>
<text x="-2.413" y="-2.54" size="0.8128" layer="21" font="vector" ratio="12">&gt;VALUE</text>
</package>
<package name="BMI-S-202">
<smd name="P2" x="0" y="-8.15" dx="3.8" dy="0.8" layer="1" thermals="no" cream="no"/>
<smd name="P1" x="-6.375" y="-8.15" dx="4.35" dy="0.8" layer="1" rot="R180" thermals="no" cream="no"/>
<smd name="P12" x="-8.15" y="-6.375" dx="4.36" dy="0.8" layer="1" rot="R90" thermals="no" cream="no"/>
<wire x1="-3.8" y1="8.25" x2="-2.1" y2="8.25" width="0.1016" layer="21"/>
<smd name="P3" x="6.375" y="-8.15" dx="4.35" dy="0.8" layer="1" rot="R180" thermals="no" cream="no"/>
<smd name="P4" x="8.15" y="-6.375" dx="4.36" dy="0.8" layer="1" rot="R90" thermals="no" cream="no"/>
<smd name="P7" x="6.375" y="8.15" dx="4.35" dy="0.8" layer="1" rot="R180" thermals="no" cream="no"/>
<smd name="P9" x="-6.375" y="8.15" dx="4.35" dy="0.8" layer="1" rot="R180" thermals="no" cream="no"/>
<smd name="P10" x="-8.15" y="6.375" dx="4.36" dy="0.8" layer="1" rot="R90" thermals="no" cream="no"/>
<smd name="P6" x="8.15" y="6.375" dx="4.36" dy="0.8" layer="1" rot="R90" thermals="no" cream="no"/>
<smd name="P8" x="0" y="8.15" dx="3.8" dy="0.8" layer="1" thermals="no" cream="no"/>
<smd name="P11" x="-8.15" y="0" dx="3.8" dy="0.8" layer="1" rot="R90" thermals="no" cream="no"/>
<smd name="P5" x="8.15" y="0" dx="3.8" dy="0.8" layer="1" rot="R90" thermals="no" cream="no"/>
<wire x1="2.1" y1="8.25" x2="3.8" y2="8.25" width="0.1016" layer="21"/>
<wire x1="2.1" y1="-8.25" x2="3.8" y2="-8.25" width="0.1016" layer="21"/>
<wire x1="-3.8" y1="-8.25" x2="-2.1" y2="-8.25" width="0.1016" layer="21"/>
<wire x1="-8.25" y1="2.1" x2="-8.25" y2="3.8" width="0.1016" layer="21"/>
<wire x1="-8.25" y1="-3.8" x2="-8.25" y2="-2.1" width="0.1016" layer="21"/>
<wire x1="8.25" y1="2.1" x2="8.25" y2="3.8" width="0.1016" layer="21"/>
<wire x1="8.25" y1="-3.8" x2="8.25" y2="-2.1" width="0.1016" layer="21"/>
<wire x1="-8.25" y1="8.25" x2="-8.25" y2="-8.25" width="0.1016" layer="51"/>
<wire x1="-8.25" y1="-8.25" x2="8.25" y2="-8.25" width="0.1016" layer="51"/>
<wire x1="8.25" y1="-8.25" x2="8.25" y2="8.25" width="0.1016" layer="51"/>
<wire x1="8.25" y1="8.25" x2="-8.25" y2="8.25" width="0.1016" layer="51"/>
</package>
<package name="CPAD_2.2MM">
<pad name="EP" x="0" y="0" drill="2.2" diameter="4.6" thermals="no"/>
</package>
<package name="BIAS_T">
<wire x1="-1.905" y1="1.905" x2="-1.905" y2="-1.905" width="0.1016" layer="21"/>
<wire x1="1.905" y1="-1.905" x2="1.905" y2="1.905" width="0.1016" layer="21"/>
<smd name="2" x="1.27" y="1.397" dx="0.762" dy="1.27" layer="1" thermals="no"/>
<smd name="3" x="1.27" y="-1.397" dx="0.762" dy="1.27" layer="1" thermals="no"/>
<smd name="4" x="-1.27" y="-1.397" dx="0.762" dy="1.27" layer="1" thermals="no"/>
<smd name="1" x="-1.27" y="1.397" dx="0.762" dy="1.27" layer="1" thermals="no"/>
<circle x="-1.27" y="0.254" radius="0.2032" width="0.1016" layer="21"/>
<wire x1="-1.905" y1="1.905" x2="-1.778" y2="1.905" width="0.1016" layer="21"/>
<wire x1="-0.762" y1="1.905" x2="0.762" y2="1.905" width="0.1016" layer="21"/>
<wire x1="-0.762" y1="-1.905" x2="0.762" y2="-1.905" width="0.1016" layer="21"/>
<wire x1="1.905" y1="1.905" x2="1.778" y2="1.905" width="0.1016" layer="21"/>
<wire x1="1.905" y1="-1.905" x2="1.778" y2="-1.905" width="0.1016" layer="21"/>
<wire x1="-1.905" y1="-1.905" x2="-1.778" y2="-1.905" width="0.1016" layer="21"/>
<text x="-2.159" y="2.413" size="1.016" layer="25" font="vector" ratio="12">&gt;NAME</text>
</package>
</packages>
<symbols>
<symbol name="LDO_SOT">
<wire x1="-7.62" y1="5.08" x2="7.62" y2="5.08" width="0.254" layer="94"/>
<wire x1="7.62" y1="5.08" x2="7.62" y2="-5.08" width="0.254" layer="94"/>
<wire x1="7.62" y1="-5.08" x2="-7.62" y2="-5.08" width="0.254" layer="94"/>
<wire x1="-7.62" y1="-5.08" x2="-7.62" y2="5.08" width="0.254" layer="94"/>
<text x="-10.16" y="6.35" size="1.778" layer="95">&gt;NAME</text>
<text x="0" y="6.35" size="1.778" layer="96">&gt;VALUE</text>
<pin name="IN" x="-10.16" y="2.54" length="short"/>
<pin name="EN" x="-10.16" y="-2.54" length="short"/>
<pin name="OUT" x="10.16" y="2.54" length="short" rot="R180"/>
<pin name="NC" x="10.16" y="-2.54" length="short" rot="R180"/>
<pin name="GND" x="0" y="-7.62" length="short" rot="R90"/>
</symbol>
<symbol name="RF_DIV">
<wire x1="-10.16" y1="-10.16" x2="10.16" y2="-10.16" width="0.4064" layer="94"/>
<wire x1="10.16" y1="-10.16" x2="10.16" y2="10.16" width="0.4064" layer="94"/>
<wire x1="10.16" y1="10.16" x2="-10.16" y2="10.16" width="0.4064" layer="94"/>
<wire x1="-10.16" y1="10.16" x2="-10.16" y2="-10.16" width="0.4064" layer="94"/>
<text x="-8.89" y="11.43" size="1.778" layer="95">&gt;NAME</text>
<text x="1.524" y="11.43" size="1.778" layer="96">&gt;VALUE</text>
<pin name="INPUT" x="-12.7" y="0" length="short" direction="in"/>
<pin name="GND@1" x="-12.7" y="5.08" length="short"/>
<pin name="GND@2" x="-12.7" y="-5.08" length="short"/>
<pin name="OUT2" x="12.7" y="5.08" length="short" rot="R180"/>
<pin name="OUT1" x="12.7" y="-5.08" length="short" rot="R180"/>
<pin name="GND@3" x="12.7" y="0" length="short" rot="R180"/>
</symbol>
<symbol name="XTAL_SMD">
<wire x1="-0.635" y1="-1.905" x2="0.635" y2="-1.905" width="0.254" layer="94"/>
<wire x1="0.635" y1="-1.905" x2="0.635" y2="1.905" width="0.254" layer="94"/>
<wire x1="0.635" y1="1.905" x2="-0.635" y2="1.905" width="0.254" layer="94"/>
<wire x1="-0.635" y1="1.905" x2="-0.635" y2="-1.905" width="0.254" layer="94"/>
<wire x1="-1.5875" y1="-1.905" x2="-1.5875" y2="0" width="0.254" layer="94"/>
<wire x1="-1.5875" y1="0" x2="-1.5875" y2="1.905" width="0.254" layer="94"/>
<wire x1="1.5875" y1="-1.905" x2="1.5875" y2="0" width="0.254" layer="94"/>
<wire x1="1.5875" y1="0" x2="1.5875" y2="1.905" width="0.254" layer="94"/>
<wire x1="-2.54" y1="0" x2="-1.5875" y2="0" width="0.127" layer="94"/>
<wire x1="1.5875" y1="0" x2="2.54" y2="0" width="0.127" layer="94"/>
<text x="-7.62" y="3.81" size="1.778" layer="95">&gt;NAME</text>
<text x="2.54" y="3.81" size="1.778" layer="95">&gt;VALUE</text>
<pin name="XTAL2" x="5.08" y="0" visible="off" length="short" direction="pas" rot="R180"/>
<pin name="XTAL1" x="-5.08" y="0" visible="off" length="short" direction="pas"/>
<pin name="GND" x="0" y="-5.08" visible="off" length="short" direction="pas" rot="R90"/>
</symbol>
<symbol name="CPAD">
<circle x="0" y="0" radius="2.54" width="0.254" layer="94"/>
<pin name="P$1" x="-5.08" y="0" visible="off" length="short" direction="sup"/>
</symbol>
<symbol name="TCXO">
<wire x1="-7.62" y1="5.08" x2="-7.62" y2="-5.08" width="0.254" layer="94"/>
<wire x1="-7.62" y1="-5.08" x2="7.62" y2="-5.08" width="0.254" layer="94"/>
<wire x1="7.62" y1="-5.08" x2="7.62" y2="5.08" width="0.254" layer="94"/>
<wire x1="7.62" y1="5.08" x2="-7.62" y2="5.08" width="0.254" layer="94"/>
<text x="-5.08" y="7.62" size="1.778" layer="95" ratio="20">&gt;NAME</text>
<text x="-5.08" y="-10.16" size="1.778" layer="96" ratio="20">&gt;VALUE</text>
<pin name="GND" x="-10.16" y="-2.54" length="short"/>
<pin name="NC" x="10.16" y="-2.54" length="short" rot="R180"/>
<pin name="VDD" x="-10.16" y="2.54" length="short"/>
<pin name="OUT" x="10.16" y="2.54" length="short" rot="R180"/>
</symbol>
<symbol name="EMI_SHIELD">
<wire x1="-5.08" y1="5.08" x2="-5.08" y2="-5.08" width="0.254" layer="94"/>
<wire x1="-5.08" y1="-5.08" x2="5.08" y2="-5.08" width="0.254" layer="94"/>
<wire x1="5.08" y1="-5.08" x2="5.08" y2="5.08" width="0.254" layer="94"/>
<wire x1="5.08" y1="5.08" x2="-5.08" y2="5.08" width="0.254" layer="94"/>
<pin name="GND" x="10.16" y="0" length="middle" rot="R180"/>
<text x="-5.08" y="-7.62" size="1.27" layer="95">EMI_SHIELD</text>
</symbol>
<symbol name="BIAS_T">
<wire x1="-10.16" y1="-10.16" x2="10.16" y2="-10.16" width="0.4064" layer="94"/>
<wire x1="10.16" y1="-10.16" x2="10.16" y2="10.16" width="0.4064" layer="94"/>
<wire x1="10.16" y1="10.16" x2="-10.16" y2="10.16" width="0.4064" layer="94"/>
<wire x1="-10.16" y1="10.16" x2="-10.16" y2="-10.16" width="0.4064" layer="94"/>
<text x="-8.89" y="11.43" size="1.778" layer="95">&gt;NAME</text>
<text x="1.524" y="11.43" size="1.778" layer="96">&gt;VALUE</text>
<pin name="DC" x="-12.7" y="5.08" length="short"/>
<pin name="RF" x="-12.7" y="-5.08" length="short"/>
<pin name="ISOLATE" x="12.7" y="5.08" length="short" rot="R180"/>
<pin name="RF&amp;DC" x="12.7" y="-5.08" length="short" rot="R180"/>
</symbol>
</symbols>
<devicesets>
<deviceset name="LDO_SOT">
<gates>
<gate name="G$1" symbol="LDO_SOT" x="0" y="0"/>
</gates>
<devices>
<device name="" package="SOT23">
<connects>
<connect gate="G$1" pin="EN" pad="3"/>
<connect gate="G$1" pin="GND" pad="2"/>
<connect gate="G$1" pin="IN" pad="1"/>
<connect gate="G$1" pin="NC" pad="4"/>
<connect gate="G$1" pin="OUT" pad="5"/>
</connects>
<technologies>
<technology name=""/>
</technologies>
</device>
</devices>
</deviceset>
<deviceset name="RF_DIV">
<gates>
<gate name="G$1" symbol="RF_DIV" x="0" y="0"/>
</gates>
<devices>
<device name="" package="RF_DIV">
<connects>
<connect gate="G$1" pin="GND@1" pad="1"/>
<connect gate="G$1" pin="GND@2" pad="3"/>
<connect gate="G$1" pin="GND@3" pad="5"/>
<connect gate="G$1" pin="INPUT" pad="2"/>
<connect gate="G$1" pin="OUT1" pad="4"/>
<connect gate="G$1" pin="OUT2" pad="6"/>
</connects>
<technologies>
<technology name=""/>
</technologies>
</device>
</devices>
</deviceset>
<deviceset name="XTAL_SMD">
<gates>
<gate name="G$1" symbol="XTAL_SMD" x="0" y="0"/>
</gates>
<devices>
<device name="" package="XTAL_SMD">
<connects>
<connect gate="G$1" pin="GND" pad="P2 P4"/>
<connect gate="G$1" pin="XTAL1" pad="P1"/>
<connect gate="G$1" pin="XTAL2" pad="P3"/>
</connects>
<technologies>
<technology name=""/>
</technologies>
</device>
</devices>
</deviceset>
<deviceset name="TCXO">
<gates>
<gate name="G$1" symbol="TCXO" x="0" y="0"/>
</gates>
<devices>
<device name="" package="TCXO">
<connects>
<connect gate="G$1" pin="GND" pad="2"/>
<connect gate="G$1" pin="NC" pad="1"/>
<connect gate="G$1" pin="OUT" pad="3"/>
<connect gate="G$1" pin="VDD" pad="4"/>
</connects>
<technologies>
<technology name=""/>
</technologies>
</device>
</devices>
</deviceset>
<deviceset name="BMI-S-202">
<gates>
<gate name="G$1" symbol="EMI_SHIELD" x="0" y="0"/>
</gates>
<devices>
<device name="" package="BMI-S-202">
<connects>
<connect gate="G$1" pin="GND" pad="P1 P2 P3 P4 P5 P6 P7 P8 P9 P10 P11 P12"/>
</connects>
<technologies>
<technology name=""/>
</technologies>
</device>
</devices>
</deviceset>
<deviceset name="CPAD_2.2MM">
<gates>
<gate name="G$1" symbol="CPAD" x="0" y="0"/>
</gates>
<devices>
<device name="" package="CPAD_2.2MM">
<connects>
<connect gate="G$1" pin="P$1" pad="EP"/>
</connects>
<technologies>
<technology name=""/>
</technologies>
</device>
</devices>
</deviceset>
<deviceset name="BIAS_T">
<gates>
<gate name="G$1" symbol="BIAS_T" x="0" y="0"/>
</gates>
<devices>
<device name="" package="BIAS_T">
<connects>
<connect gate="G$1" pin="DC" pad="1"/>
<connect gate="G$1" pin="ISOLATE" pad="2"/>
<connect gate="G$1" pin="RF" pad="4"/>
<connect gate="G$1" pin="RF&amp;DC" pad="3"/>
</connects>
<technologies>
<technology name=""/>
</technologies>
</device>
</devices>
</deviceset>
</devicesets>
</library>
<library name="ttaka_desc">
<packages>
<package name="C3225">
<smd name="P$1" x="0" y="1.6" dx="1.6" dy="1.2" layer="1" thermals="no"/>
<smd name="P$2" x="0" y="-1.6" dx="1.6" dy="1.2" layer="1" thermals="no"/>
<text x="-1.4" y="-1.6" size="0.8128" layer="25" font="vector" ratio="12" rot="R90">&gt;NAME</text>
<wire x1="-0.8" y1="1.6" x2="-0.8" y2="-1.6" width="0.1016" layer="51"/>
<wire x1="-0.8" y1="-1.6" x2="0.8" y2="-1.6" width="0.1016" layer="51"/>
<wire x1="0.8" y1="-1.6" x2="0.8" y2="1.6" width="0.1016" layer="51"/>
<wire x1="0.8" y1="1.6" x2="-0.8" y2="1.6" width="0.1016" layer="51"/>
<wire x1="-0.9" y1="2.3" x2="-0.9" y2="-2.3" width="0.1016" layer="21"/>
<wire x1="0.9" y1="2.3" x2="0.9" y2="-2.3" width="0.1016" layer="21"/>
<wire x1="-0.9" y1="2.3" x2="0.9" y2="2.3" width="0.1016" layer="21"/>
<wire x1="-0.9" y1="-2.3" x2="0.9" y2="-2.3" width="0.1016" layer="21"/>
</package>
<package name="C1005">
<wire x1="0.5" y1="0.25" x2="0.5" y2="-0.25" width="0.1016" layer="51"/>
<wire x1="0.5" y1="-0.25" x2="-0.5" y2="-0.25" width="0.1016" layer="51"/>
<wire x1="-0.5" y1="-0.25" x2="-0.5" y2="0.25" width="0.1016" layer="51"/>
<smd name="2" x="0.5" y="0" dx="0.45" dy="0.6" layer="1"/>
<smd name="1" x="-0.5" y="0" dx="0.45" dy="0.6" layer="1"/>
<wire x1="-0.5" y1="0.25" x2="0.5" y2="0.25" width="0.1016" layer="51"/>
<wire x1="-0.85" y1="0.4" x2="0.85" y2="0.4" width="0.1016" layer="21"/>
<wire x1="-0.85" y1="-0.4" x2="0.85" y2="-0.4" width="0.1016" layer="21"/>
<wire x1="-0.85" y1="-0.4" x2="-0.85" y2="0.4" width="0.1016" layer="21"/>
<wire x1="0.85" y1="-0.4" x2="0.85" y2="0.4" width="0.1016" layer="21"/>
<text x="-1.7" y="0.65" size="0.8128" layer="25" font="vector" ratio="12">&gt;NAME</text>
</package>
<package name="LED_1608">
<description>&lt;b&gt;CAPACITOR&lt;/b&gt;</description>
<wire x1="-1.3" y1="0.5" x2="1.3" y2="0.5" width="0.1016" layer="21"/>
<wire x1="-1.3" y1="-0.5" x2="1.3" y2="-0.5" width="0.1016" layer="21"/>
<smd name="1" x="-0.8" y="0" dx="0.8" dy="0.8" layer="1"/>
<smd name="2" x="0.8" y="0" dx="0.8" dy="0.8" layer="1"/>
<text x="-1.635" y="1.143" size="0.8128" layer="25" font="vector" ratio="12">&gt;NAME</text>
<wire x1="-1.3" y1="-0.5" x2="-1.3" y2="0.5" width="0.1016" layer="21"/>
<wire x1="1.3" y1="-0.5" x2="1.3" y2="0.5" width="0.1016" layer="21"/>
<polygon width="0.1016" layer="21">
<vertex x="-0.2" y="0.4"/>
<vertex x="-0.2" y="-0.4"/>
<vertex x="0.2" y="0"/>
</polygon>
<wire x1="0.2" y1="0.4" x2="0.2" y2="-0.4" width="0.127" layer="21"/>
</package>
<package name="EXB28V">
<description>&lt;b&gt;CAPACITOR&lt;/b&gt;</description>
<smd name="4" x="-0.25" y="0.5" dx="0.25" dy="0.5" layer="1"/>
<text x="-2.5" y="1.5" size="1.27" layer="25" font="vector" ratio="10">&gt;NAME</text>
<smd name="6" x="0.25" y="0.5" dx="0.25" dy="0.5" layer="1"/>
<wire x1="-1" y1="0.5" x2="-1" y2="-0.5" width="0.127" layer="51"/>
<wire x1="-1" y1="-0.5" x2="1" y2="-0.5" width="0.127" layer="51"/>
<wire x1="1" y1="-0.5" x2="1" y2="0.5" width="0.127" layer="51"/>
<text x="-2.5" y="-2.5" size="1.27" layer="25" font="vector" ratio="10">&gt;VALUE</text>
<smd name="8" x="0.8" y="0.5" dx="0.35" dy="0.5" layer="1"/>
<smd name="2" x="-0.8" y="0.5" dx="0.35" dy="0.5" layer="1"/>
<smd name="5" x="0.25" y="-0.5" dx="0.25" dy="0.5" layer="1"/>
<smd name="7" x="0.8" y="-0.5" dx="0.35" dy="0.5" layer="1"/>
<smd name="3" x="-0.25" y="-0.5" dx="0.25" dy="0.5" layer="1"/>
<smd name="1" x="-0.8" y="-0.5" dx="0.35" dy="0.5" layer="1"/>
<wire x1="-1" y1="0.5" x2="1" y2="0.5" width="0.127" layer="51"/>
<wire x1="-1" y1="0.1" x2="-1" y2="-0.1" width="0.127" layer="21"/>
<wire x1="1" y1="0.1" x2="1" y2="-0.1" width="0.127" layer="21"/>
</package>
<package name="C1608">
<description>&lt;b&gt;CAPACITOR&lt;/b&gt;</description>
<wire x1="-0.8" y1="0.4" x2="0.8" y2="0.4" width="0.1016" layer="51"/>
<wire x1="-0.8" y1="-0.4" x2="0.8" y2="-0.4" width="0.1016" layer="51"/>
<smd name="1" x="-0.8" y="0" dx="0.8" dy="0.8" layer="1" thermals="no"/>
<smd name="2" x="0.8" y="0" dx="0.8" dy="0.8" layer="1" thermals="no"/>
<text x="-1.5" y="1" size="0.8128" layer="25" font="vector" ratio="12">&gt;NAME</text>
<wire x1="-0.8" y1="-0.4" x2="-0.8" y2="0.4" width="0.1016" layer="51"/>
<wire x1="0.8" y1="-0.4" x2="0.8" y2="0.4" width="0.1016" layer="51"/>
<wire x1="-1.3" y1="0.5" x2="1.3" y2="0.5" width="0.1016" layer="21"/>
<wire x1="-1.3" y1="-0.5" x2="1.3" y2="-0.5" width="0.1016" layer="21"/>
<wire x1="-1.3" y1="0.5" x2="-1.3" y2="-0.5" width="0.1016" layer="21"/>
<wire x1="1.3" y1="0.5" x2="1.3" y2="-0.5" width="0.1016" layer="21"/>
</package>
</packages>
<symbols>
<symbol name="CAPACITOR">
<wire x1="-1.905" y1="-0.635" x2="0" y2="-0.635" width="0.508" layer="94"/>
<wire x1="0" y1="-0.635" x2="1.905" y2="-0.635" width="0.508" layer="94"/>
<wire x1="-1.905" y1="0.635" x2="0" y2="0.635" width="0.508" layer="94"/>
<wire x1="0" y1="0.635" x2="1.905" y2="0.635" width="0.508" layer="94"/>
<wire x1="0" y1="2.54" x2="0" y2="0.635" width="0.254" layer="94"/>
<wire x1="0" y1="-0.635" x2="0" y2="-2.54" width="0.254" layer="94"/>
<text x="2.54" y="2.54" size="1.27" layer="95">&gt;NAME</text>
<text x="2.54" y="-2.54" size="1.27" layer="96">&gt;VALUE</text>
<pin name="2" x="0" y="-2.54" visible="off" length="point" direction="pas" rot="R90"/>
<pin name="1" x="0" y="2.54" visible="off" length="point" direction="pas" rot="R270"/>
</symbol>
<symbol name="LED">
<wire x1="-2.54" y1="0" x2="5.1435" y2="0" width="0.254" layer="94"/>
<text x="-1.27" y="-5.08" size="1.27" layer="95">&gt;NAME</text>
<pin name="+" x="-2.54" y="0" visible="off" length="point" direction="pas"/>
<pin name="P$2" x="5.08" y="0" visible="off" length="point" direction="pas"/>
<polygon width="0.254" layer="94">
<vertex x="-0.762" y="1.778"/>
<vertex x="-0.762" y="-2.032"/>
<vertex x="2.286" y="0"/>
</polygon>
<wire x1="2.54" y1="1.778" x2="2.54" y2="-1.778" width="0.508" layer="94"/>
<wire x1="0.254" y1="2.54" x2="1.016" y2="4.318" width="0.254" layer="94"/>
<wire x1="1.016" y1="4.318" x2="1.524" y2="2.794" width="0.254" layer="94"/>
<wire x1="1.524" y1="2.794" x2="2.286" y2="4.572" width="0.254" layer="94"/>
</symbol>
<symbol name="ARRAY_REG_4P">
<text x="-2.54" y="7.62" size="1.27" layer="95">&gt;NAME</text>
<text x="-2.54" y="-13.97" size="1.27" layer="96">&gt;VALUE</text>
<wire x1="-2.54" y1="0" x2="-2.2225" y2="1.27" width="0.254" layer="94"/>
<wire x1="-2.2225" y1="1.27" x2="-1.5875" y2="-1.27" width="0.254" layer="94"/>
<wire x1="-1.5875" y1="-1.27" x2="-0.9525" y2="1.27" width="0.254" layer="94"/>
<wire x1="-0.9525" y1="1.27" x2="-0.3175" y2="-1.27" width="0.254" layer="94"/>
<wire x1="-0.3175" y1="-1.27" x2="0.3175" y2="1.27" width="0.254" layer="94"/>
<wire x1="0.3175" y1="1.27" x2="0.9525" y2="-1.27" width="0.254" layer="94"/>
<wire x1="0.9525" y1="-1.27" x2="1.5875" y2="1.27" width="0.254" layer="94"/>
<wire x1="1.5875" y1="1.27" x2="2.2225" y2="-1.27" width="0.254" layer="94"/>
<wire x1="2.2225" y1="-1.27" x2="2.54" y2="0" width="0.254" layer="94"/>
<pin name="P3" x="-2.54" y="0" visible="off" length="point" direction="pas"/>
<pin name="P4" x="2.54" y="0" visible="off" length="point" direction="pas"/>
<wire x1="-2.54" y1="-5.08" x2="-2.2225" y2="-3.81" width="0.254" layer="94"/>
<wire x1="-2.2225" y1="-3.81" x2="-1.5875" y2="-6.35" width="0.254" layer="94"/>
<wire x1="-1.5875" y1="-6.35" x2="-0.9525" y2="-3.81" width="0.254" layer="94"/>
<wire x1="-0.9525" y1="-3.81" x2="-0.3175" y2="-6.35" width="0.254" layer="94"/>
<wire x1="-0.3175" y1="-6.35" x2="0.3175" y2="-3.81" width="0.254" layer="94"/>
<wire x1="0.3175" y1="-3.81" x2="0.9525" y2="-6.35" width="0.254" layer="94"/>
<wire x1="0.9525" y1="-6.35" x2="1.5875" y2="-3.81" width="0.254" layer="94"/>
<wire x1="1.5875" y1="-3.81" x2="2.2225" y2="-6.35" width="0.254" layer="94"/>
<wire x1="2.2225" y1="-6.35" x2="2.54" y2="-5.08" width="0.254" layer="94"/>
<pin name="P5" x="-2.54" y="-5.08" visible="off" length="point" direction="pas"/>
<pin name="P6" x="2.54" y="-5.08" visible="off" length="point" direction="pas"/>
<wire x1="-2.54" y1="5.08" x2="-2.2225" y2="6.35" width="0.254" layer="94"/>
<wire x1="-2.2225" y1="6.35" x2="-1.5875" y2="3.81" width="0.254" layer="94"/>
<wire x1="-1.5875" y1="3.81" x2="-0.9525" y2="6.35" width="0.254" layer="94"/>
<wire x1="-0.9525" y1="6.35" x2="-0.3175" y2="3.81" width="0.254" layer="94"/>
<wire x1="-0.3175" y1="3.81" x2="0.3175" y2="6.35" width="0.254" layer="94"/>
<wire x1="0.3175" y1="6.35" x2="0.9525" y2="3.81" width="0.254" layer="94"/>
<wire x1="0.9525" y1="3.81" x2="1.5875" y2="6.35" width="0.254" layer="94"/>
<wire x1="1.5875" y1="6.35" x2="2.2225" y2="3.81" width="0.254" layer="94"/>
<wire x1="2.2225" y1="3.81" x2="2.54" y2="5.08" width="0.254" layer="94"/>
<pin name="P1" x="-2.54" y="5.08" visible="off" length="point" direction="pas"/>
<pin name="P2" x="2.54" y="5.08" visible="off" length="point" direction="pas"/>
<wire x1="-2.54" y1="-10.16" x2="-2.2225" y2="-8.89" width="0.254" layer="94"/>
<wire x1="-2.2225" y1="-8.89" x2="-1.5875" y2="-11.43" width="0.254" layer="94"/>
<wire x1="-1.5875" y1="-11.43" x2="-0.9525" y2="-8.89" width="0.254" layer="94"/>
<wire x1="-0.9525" y1="-8.89" x2="-0.3175" y2="-11.43" width="0.254" layer="94"/>
<wire x1="-0.3175" y1="-11.43" x2="0.3175" y2="-8.89" width="0.254" layer="94"/>
<wire x1="0.3175" y1="-8.89" x2="0.9525" y2="-11.43" width="0.254" layer="94"/>
<wire x1="0.9525" y1="-11.43" x2="1.5875" y2="-8.89" width="0.254" layer="94"/>
<wire x1="1.5875" y1="-8.89" x2="2.2225" y2="-11.43" width="0.254" layer="94"/>
<wire x1="2.2225" y1="-11.43" x2="2.54" y2="-10.16" width="0.254" layer="94"/>
<pin name="P7" x="-2.54" y="-10.16" visible="off" length="point" direction="pas"/>
<pin name="P8" x="2.54" y="-10.16" visible="off" length="point" direction="pas"/>
</symbol>
<symbol name="RESISTOR">
<wire x1="-2.54" y1="0" x2="-2.2225" y2="1.27" width="0.254" layer="94"/>
<wire x1="-2.2225" y1="1.27" x2="-1.5875" y2="-1.27" width="0.254" layer="94"/>
<wire x1="-1.5875" y1="-1.27" x2="-0.9525" y2="1.27" width="0.254" layer="94"/>
<wire x1="-0.9525" y1="1.27" x2="-0.3175" y2="-1.27" width="0.254" layer="94"/>
<wire x1="-0.3175" y1="-1.27" x2="0.3175" y2="1.27" width="0.254" layer="94"/>
<wire x1="0.3175" y1="1.27" x2="0.9525" y2="-1.27" width="0.254" layer="94"/>
<wire x1="0.9525" y1="-1.27" x2="1.5875" y2="1.27" width="0.254" layer="94"/>
<wire x1="1.5875" y1="1.27" x2="2.2225" y2="-1.27" width="0.254" layer="94"/>
<wire x1="2.2225" y1="-1.27" x2="2.54" y2="0" width="0.254" layer="94"/>
<text x="-4.826" y="2.032" size="1.27" layer="95">&gt;NAME</text>
<text x="0" y="2.032" size="1.27" layer="96">&gt;VALUE</text>
<pin name="P$1" x="-2.54" y="0" visible="off" length="point" direction="pas"/>
<pin name="P$2" x="2.54" y="0" visible="off" length="point" direction="pas"/>
</symbol>
<symbol name="POLYSW">
<wire x1="-5.08" y1="2.54" x2="-5.08" y2="-2.54" width="0.254" layer="94"/>
<wire x1="-5.08" y1="-2.54" x2="5.08" y2="-2.54" width="0.254" layer="94"/>
<wire x1="5.08" y1="-2.54" x2="5.08" y2="2.54" width="0.254" layer="94"/>
<wire x1="5.08" y1="2.54" x2="-5.08" y2="2.54" width="0.254" layer="94"/>
<pin name="P$1" x="-10.16" y="0" visible="pad" length="middle"/>
<pin name="P$2" x="10.16" y="0" visible="pad" length="middle" rot="R180"/>
<text x="-5.08" y="5.08" size="1.778" layer="95">&gt;NAME</text>
<text x="-5.08" y="-5.08" size="1.778" layer="96">&gt;VALUE</text>
</symbol>
</symbols>
<devicesets>
<deviceset name="C3225">
<gates>
<gate name="G$1" symbol="CAPACITOR" x="0" y="0"/>
</gates>
<devices>
<device name="" package="C3225">
<connects>
<connect gate="G$1" pin="1" pad="P$1"/>
<connect gate="G$1" pin="2" pad="P$2"/>
</connects>
<technologies>
<technology name=""/>
</technologies>
</device>
</devices>
</deviceset>
<deviceset name="C1005">
<gates>
<gate name="G$1" symbol="CAPACITOR" x="0" y="0"/>
</gates>
<devices>
<device name="" package="C1005">
<connects>
<connect gate="G$1" pin="1" pad="1"/>
<connect gate="G$1" pin="2" pad="2"/>
</connects>
<technologies>
<technology name=""/>
</technologies>
</device>
</devices>
</deviceset>
<deviceset name="LED1608">
<gates>
<gate name="G$1" symbol="LED" x="0" y="0"/>
</gates>
<devices>
<device name="" package="LED_1608">
<connects>
<connect gate="G$1" pin="+" pad="1"/>
<connect gate="G$1" pin="P$2" pad="2"/>
</connects>
<technologies>
<technology name=""/>
</technologies>
</device>
</devices>
</deviceset>
<deviceset name="ARRAY_REG__EXB28V">
<gates>
<gate name="G$1" symbol="ARRAY_REG_4P" x="0" y="2.54"/>
</gates>
<devices>
<device name="" package="EXB28V">
<connects>
<connect gate="G$1" pin="P1" pad="1"/>
<connect gate="G$1" pin="P2" pad="2"/>
<connect gate="G$1" pin="P3" pad="3"/>
<connect gate="G$1" pin="P4" pad="4"/>
<connect gate="G$1" pin="P5" pad="5"/>
<connect gate="G$1" pin="P6" pad="6"/>
<connect gate="G$1" pin="P7" pad="7"/>
<connect gate="G$1" pin="P8" pad="8"/>
</connects>
<technologies>
<technology name=""/>
</technologies>
</device>
</devices>
</deviceset>
<deviceset name="R1005">
<gates>
<gate name="G$1" symbol="RESISTOR" x="0" y="0"/>
</gates>
<devices>
<device name="" package="C1005">
<connects>
<connect gate="G$1" pin="P$1" pad="1"/>
<connect gate="G$1" pin="P$2" pad="2"/>
</connects>
<technologies>
<technology name=""/>
</technologies>
</device>
</devices>
</deviceset>
<deviceset name="POLYSW1608">
<gates>
<gate name="G$1" symbol="POLYSW" x="0" y="0"/>
</gates>
<devices>
<device name="" package="C1608">
<connects>
<connect gate="G$1" pin="P$1" pad="1"/>
<connect gate="G$1" pin="P$2" pad="2"/>
</connects>
<technologies>
<technology name=""/>
</technologies>
</device>
</devices>
</deviceset>
</devicesets>
</library>
<library name="ttaka_conn">
<packages>
<package name="MICROUSB">
<smd name="3" x="0" y="4.125" dx="0.4" dy="1.35" layer="1" rot="R180"/>
<smd name="4" x="0.65" y="4.125" dx="0.4" dy="1.35" layer="1" rot="R180"/>
<smd name="5" x="1.3" y="4.125" dx="0.4" dy="1.35" layer="1" rot="R180" thermals="no"/>
<smd name="2" x="-0.65" y="4.125" dx="0.4" dy="1.35" layer="1" rot="R180"/>
<smd name="1" x="-1.3" y="4.125" dx="0.4" dy="1.35" layer="1" rot="R180" thermals="no"/>
<smd name="G4" x="3.1" y="4" dx="2.1" dy="1.6" layer="1" rot="R180"/>
<smd name="G1" x="-3.7" y="1.45" dx="1.9" dy="1.9" layer="1"/>
<smd name="G5" x="-1.2" y="1.45" dx="1.9" dy="1.9" layer="1"/>
<smd name="G3" x="-3.1" y="4" dx="2.1" dy="1.6" layer="1" rot="R180"/>
<smd name="G6" x="1.2" y="1.45" dx="1.9" dy="1.9" layer="1"/>
<smd name="G2" x="3.7" y="1.45" dx="1.9" dy="1.9" layer="1"/>
<wire x1="-5.5" y1="0" x2="5.5" y2="0" width="0.0762" layer="51" style="dashdot"/>
<wire x1="-5" y1="1.45" x2="-5.5" y2="1.45" width="0.1016" layer="21"/>
<wire x1="5.5" y1="1.45" x2="5" y2="1.45" width="0.1016" layer="21"/>
<wire x1="0" y1="0.3" x2="0" y2="0" width="0.1016" layer="21"/>
<wire x1="0" y1="3.1" x2="0" y2="2.6" width="0.1016" layer="21"/>
<wire x1="-3.75" y1="4.3" x2="-3.75" y2="1.9" width="0.1016" layer="51"/>
<wire x1="-3.75" y1="0.3" x2="-3.75" y2="0" width="0.127" layer="21"/>
<wire x1="3.75" y1="0" x2="3.75" y2="0.3" width="0.1016" layer="21"/>
<wire x1="3.75" y1="2.6" x2="3.75" y2="3.1" width="0.1016" layer="21"/>
<wire x1="-1.9" y1="4.3" x2="-1.6" y2="4.3" width="0.1016" layer="21"/>
<wire x1="1.9" y1="4.3" x2="1.6" y2="4.3" width="0.1016" layer="21"/>
<wire x1="-3.95" y1="1.9" x2="-3.95" y2="1.1" width="0.1016" layer="51"/>
<wire x1="-3.95" y1="1.9" x2="-3.75" y2="1.9" width="0.1016" layer="51"/>
<wire x1="-3.75" y1="1.1" x2="-3.75" y2="-0.7" width="0.1016" layer="51"/>
<wire x1="-1.9" y1="4.3" x2="-1.9" y2="4" width="0.1016" layer="51"/>
<wire x1="-3.75" y1="4.3" x2="-1.95" y2="4.3" width="0.1016" layer="51"/>
<wire x1="-3.95" y1="1.1" x2="-3.75" y2="1.1" width="0.1016" layer="51"/>
<wire x1="-3.75" y1="-0.7" x2="-4" y2="-1" width="0.1016" layer="51"/>
<wire x1="-4" y1="-1" x2="-3.8" y2="-1.2" width="0.1016" layer="51"/>
<wire x1="-3.8" y1="-1.2" x2="-3.5" y2="-0.9" width="0.1016" layer="51"/>
<wire x1="-3.5" y1="-0.9" x2="-3.3" y2="-0.9" width="0.1016" layer="51"/>
<wire x1="-3.3" y1="-0.9" x2="-3.3" y2="-1.22" width="0.1016" layer="51"/>
<wire x1="-1.9" y1="4" x2="-1.5" y2="4" width="0.1016" layer="51"/>
<wire x1="-1.5" y1="4" x2="-1.5" y2="4.4" width="0.1016" layer="51"/>
<wire x1="3.75" y1="4.3" x2="3.75" y2="1.9" width="0.1016" layer="51"/>
<wire x1="3.95" y1="1.9" x2="3.95" y2="1.1" width="0.1016" layer="51"/>
<wire x1="3.95" y1="1.9" x2="3.75" y2="1.9" width="0.1016" layer="51"/>
<wire x1="3.75" y1="1.1" x2="3.75" y2="-0.7" width="0.1016" layer="51"/>
<wire x1="1.9" y1="4.3" x2="1.9" y2="4" width="0.1016" layer="51"/>
<wire x1="3.75" y1="4.3" x2="1.95" y2="4.3" width="0.1016" layer="51"/>
<wire x1="3.95" y1="1.1" x2="3.75" y2="1.1" width="0.1016" layer="51"/>
<wire x1="3.75" y1="-0.7" x2="4" y2="-1" width="0.1016" layer="51"/>
<wire x1="4" y1="-1" x2="3.8" y2="-1.2" width="0.1016" layer="51"/>
<wire x1="3.8" y1="-1.2" x2="3.5" y2="-0.9" width="0.1016" layer="51"/>
<wire x1="3.5" y1="-0.9" x2="3.3" y2="-0.9" width="0.1016" layer="51"/>
<wire x1="3.3" y1="-0.9" x2="3.3" y2="-1.22" width="0.1016" layer="51"/>
<wire x1="3.3" y1="-1.22" x2="-3.3" y2="-1.22" width="0.1016" layer="51"/>
<wire x1="1.9" y1="4" x2="1.5" y2="4" width="0.1016" layer="51"/>
<wire x1="1.5" y1="4" x2="1.5" y2="4.4" width="0.1016" layer="51"/>
<wire x1="1.5" y1="4.4" x2="-1.5" y2="4.4" width="0.1016" layer="51"/>
<wire x1="0" y1="-2.5" x2="0" y2="5.5" width="0.1016" layer="51" style="dashdot"/>
<wire x1="-3.75" y1="2.6" x2="-3.75" y2="3.1" width="0.1016" layer="21"/>
<text x="-1.5" y="6" size="0.8128" layer="21" font="vector" ratio="12">&gt;NAME</text>
</package>
<package name="PINH-2MM-2P">
<wire x1="-1" y1="0.2" x2="-1" y2="1.8" width="0.1016" layer="21"/>
<pad name="2" x="0" y="1" drill="0.8"/>
<pad name="1" x="0" y="-1" drill="0.8"/>
<text x="-3" y="-4.5" size="0.8128" layer="27" font="vector" ratio="12">&gt;VALUE</text>
<text x="-2" y="3.5" size="0.8128" layer="25" font="vector" ratio="12">&gt;NAME</text>
<wire x1="-0.8" y1="2" x2="-1" y2="1.8" width="0.1016" layer="21"/>
<wire x1="-0.8" y1="2" x2="0.8" y2="2" width="0.1016" layer="21"/>
<wire x1="1" y1="0.2" x2="1" y2="1.8" width="0.1016" layer="21"/>
<wire x1="0.8" y1="2" x2="1" y2="1.8" width="0.1016" layer="21"/>
<wire x1="-1" y1="0.2" x2="-0.8" y2="0" width="0.1016" layer="21"/>
<wire x1="0.8" y1="0" x2="1" y2="0.2" width="0.1016" layer="21"/>
<wire x1="1" y1="-0.2" x2="1" y2="-1.8" width="0.1016" layer="21"/>
<wire x1="0.8" y1="-2" x2="1" y2="-1.8" width="0.1016" layer="21"/>
<wire x1="0.8" y1="-2" x2="-0.8" y2="-2" width="0.1016" layer="21"/>
<wire x1="1" y1="-0.2" x2="0.8" y2="0" width="0.1016" layer="21"/>
<wire x1="-1" y1="-1.8" x2="-1" y2="-0.2" width="0.1016" layer="21"/>
<wire x1="-0.8" y1="0" x2="-1" y2="-0.2" width="0.1016" layer="21"/>
<wire x1="-1" y1="-1.8" x2="-0.8" y2="-2" width="0.1016" layer="21"/>
</package>
<package name="SMA-ANG">
<description>FEMALE SMA</description>
<wire x1="-2.365" y1="-5.4" x2="-3.175" y2="-5.4" width="0.127" layer="51"/>
<text x="-3.04" y="-8.715" size="1.27" layer="25" font="vector" ratio="10">&gt;NAME</text>
<wire x1="-3.175" y1="0" x2="-3.175" y2="-1.6" width="0.127" layer="51"/>
<wire x1="-3.175" y1="-1.6" x2="-3.175" y2="-5.4" width="0.127" layer="51"/>
<wire x1="3.175" y1="-5.4" x2="3.175" y2="-1.6" width="0.127" layer="51"/>
<wire x1="3.175" y1="-1.6" x2="3.175" y2="0" width="0.127" layer="51"/>
<wire x1="3.175" y1="0" x2="2.6" y2="0" width="0.127" layer="51"/>
<wire x1="2.6" y1="0" x2="-2.6" y2="0" width="0.127" layer="51"/>
<wire x1="-2.6" y1="0" x2="-3.175" y2="0" width="0.127" layer="51"/>
<wire x1="-2.6" y1="0" x2="-2.6" y2="1.1" width="0.127" layer="51"/>
<wire x1="-2.6" y1="1.1" x2="-3.175" y2="1.5" width="0.127" layer="51"/>
<wire x1="-3.175" y1="1.5" x2="-3.175" y2="7.3" width="0.127" layer="51"/>
<wire x1="-3.175" y1="7.3" x2="-2.5" y2="7.9" width="0.127" layer="51"/>
<wire x1="-2.5" y1="7.9" x2="2.5" y2="7.9" width="0.127" layer="51"/>
<wire x1="2.5" y1="7.9" x2="3.175" y2="7.3" width="0.127" layer="51"/>
<wire x1="3.175" y1="7.3" x2="3.175" y2="1.5" width="0.127" layer="51"/>
<wire x1="3.175" y1="1.5" x2="2.6" y2="1.1" width="0.127" layer="51"/>
<wire x1="2.6" y1="1.1" x2="2.6" y2="0" width="0.127" layer="51"/>
<wire x1="3.175" y1="-1.6" x2="2.365" y2="-1.6" width="0.127" layer="51"/>
<wire x1="2.365" y1="-1.6" x2="0.35" y2="-1.6" width="0.127" layer="51"/>
<wire x1="0.35" y1="-1.6" x2="-0.35" y2="-1.6" width="0.127" layer="51"/>
<wire x1="-0.35" y1="-1.6" x2="-2.365" y2="-1.6" width="0.127" layer="51"/>
<wire x1="-2.365" y1="-1.6" x2="-3.175" y2="-1.6" width="0.127" layer="51"/>
<wire x1="-2.365" y1="-1.6" x2="-2.365" y2="-5.4" width="0.127" layer="51"/>
<wire x1="2.365" y1="-5.4" x2="2.365" y2="-1.6" width="0.127" layer="51"/>
<wire x1="-0.35" y1="-1.6" x2="-0.35" y2="-5.4" width="0.127" layer="51"/>
<wire x1="0.35" y1="-1.6" x2="0.35" y2="-5.4" width="0.127" layer="51"/>
<wire x1="3.175" y1="-5.4" x2="2.365" y2="-5.4" width="0.127" layer="51"/>
<wire x1="0.35" y1="-5.4" x2="-0.35" y2="-5.4" width="0.127" layer="51"/>
<smd name="P2" x="-2.75" y="-4" dx="4" dy="1" layer="1" rot="R270" thermals="no" cream="no"/>
<smd name="P3" x="2.75" y="-4" dx="4" dy="1" layer="1" rot="R270" thermals="no" cream="no"/>
<smd name="P1" x="0" y="-4" dx="4" dy="1" layer="1" rot="R270" thermals="no" cream="no"/>
<smd name="P4" x="-2.75" y="-4" dx="4" dy="1" layer="16" rot="R270" thermals="no" cream="no"/>
<smd name="P5" x="2.75" y="-4" dx="4" dy="1" layer="16" rot="R270" thermals="no" cream="no"/>
</package>
</packages>
<symbols>
<symbol name="MICROUSB">
<pin name="D+" x="-10.08" y="0" length="short"/>
<pin name="D-" x="-10.08" y="5.04" length="short"/>
<pin name="VBUS" x="-10.08" y="10.08" length="short" direction="sup"/>
<pin name="ID" x="-10.08" y="-5.04" length="short"/>
<pin name="GND" x="-10.08" y="-10.08" length="short" direction="sup"/>
<pin name="FP" x="10.16" y="-10.16" length="short" direction="sup" rot="R180"/>
<wire x1="-7.62" y1="12.7" x2="-7.62" y2="-12.7" width="0.254" layer="94"/>
<wire x1="-7.62" y1="-12.7" x2="7.62" y2="-12.7" width="0.254" layer="94"/>
<wire x1="7.62" y1="-12.7" x2="7.62" y2="12.7" width="0.254" layer="94"/>
<wire x1="7.62" y1="12.7" x2="-7.62" y2="12.7" width="0.254" layer="94"/>
<text x="-5.08" y="15.24" size="1.778" layer="95">&gt;NAME</text>
<text x="-5.08" y="-17.78" size="1.778" layer="96">&gt;VALUE</text>
</symbol>
<symbol name="PINH_2P">
<text x="-3.81" y="7.62" size="1.778" layer="95" ratio="5">&gt;NAME</text>
<pin name="P2" x="-4.064" y="-2.54" visible="pad" length="middle" function="dot"/>
<pin name="P1" x="-4.064" y="2.54" visible="pad" length="middle" function="dot"/>
<wire x1="2.54" y1="5.08" x2="2.54" y2="-5.08" width="0.254" layer="94"/>
<wire x1="2.54" y1="-5.08" x2="-2.54" y2="-5.08" width="0.254" layer="94"/>
<wire x1="-2.54" y1="-5.08" x2="-2.54" y2="5.08" width="0.254" layer="94"/>
<wire x1="-2.54" y1="5.08" x2="2.54" y2="5.08" width="0.254" layer="94"/>
</symbol>
<symbol name="SMA-G">
<circle x="0" y="0" radius="1.27" width="0.254" layer="94"/>
<circle x="0" y="0" radius="2.6181" width="0.254" layer="94"/>
<text x="-3.81" y="3.81" size="1.778" layer="95">&gt;NAME</text>
<pin name="RF" x="6.35" y="0" visible="off" length="middle" direction="pas" rot="R180"/>
<pin name="GND" x="0" y="-7.62" visible="off" length="middle" direction="pas" rot="R90"/>
</symbol>
</symbols>
<devicesets>
<deviceset name="MICROUSB">
<gates>
<gate name="G$1" symbol="MICROUSB" x="0" y="0"/>
</gates>
<devices>
<device name="" package="MICROUSB">
<connects>
<connect gate="G$1" pin="D+" pad="3"/>
<connect gate="G$1" pin="D-" pad="2"/>
<connect gate="G$1" pin="FP" pad="G1 G2 G3 G4 G5 G6"/>
<connect gate="G$1" pin="GND" pad="5"/>
<connect gate="G$1" pin="ID" pad="4"/>
<connect gate="G$1" pin="VBUS" pad="1"/>
</connects>
<technologies>
<technology name=""/>
</technologies>
</device>
</devices>
</deviceset>
<deviceset name="PINH_2MM_2P">
<gates>
<gate name="G$1" symbol="PINH_2P" x="0" y="0"/>
</gates>
<devices>
<device name="" package="PINH-2MM-2P">
<connects>
<connect gate="G$1" pin="P1" pad="1"/>
<connect gate="G$1" pin="P2" pad="2"/>
</connects>
<technologies>
<technology name=""/>
</technologies>
</device>
</devices>
</deviceset>
<deviceset name="SMA-ANG">
<gates>
<gate name="G$1" symbol="SMA-G" x="0" y="0"/>
</gates>
<devices>
<device name="" package="SMA-ANG">
<connects>
<connect gate="G$1" pin="GND" pad="P2 P3 P4 P5"/>
<connect gate="G$1" pin="RF" pad="P1"/>
</connects>
<technologies>
<technology name=""/>
</technologies>
</device>
</devices>
</deviceset>
</devicesets>
</library>
</libraries>
<attributes>
</attributes>
<variantdefs>
</variantdefs>
<classes>
<class number="0" name="default" width="0" drill="0">
</class>
</classes>
<parts>
<part name="U1" library="ttaka_ic" deviceset="MAX2771" device=""/>
<part name="C5" library="ttaka_desc" deviceset="C1005" device="" value="470pF"/>
<part name="C2" library="ttaka_desc" deviceset="C1005" device="" value="1.7pF"/>
<part name="L1" library="ublox" deviceset="L1608" device="" value="5.6nH"/>
<part name="U$13" library="power" deviceset="+3.3V" device=""/>
<part name="U$15" library="power" deviceset="GND" device=""/>
<part name="U$16" library="power" deviceset="GND" device=""/>
<part name="C1" library="ttaka_desc" deviceset="C1005" device="" value="470pF"/>
<part name="U$22" library="power" deviceset="+3.3V" device=""/>
<part name="U$23" library="power" deviceset="+3.3V" device=""/>
<part name="C34" library="ttaka_desc" deviceset="C1005" device="" value="0.1uF"/>
<part name="C39" library="ttaka_desc" deviceset="C1005" device="" value="100pF"/>
<part name="U$31" library="power" deviceset="GND" device=""/>
<part name="U$32" library="power" deviceset="+3.3V" device=""/>
<part name="TCXO" library="ttaka_misc" deviceset="TCXO" device="" value="24.000 MHz"/>
<part name="U$7" library="power" deviceset="GND" device=""/>
<part name="C13" library="ttaka_desc" deviceset="C1005" device="" value="0.1uF"/>
<part name="C8" library="ttaka_desc" deviceset="C1005" device="" value="0.01uF"/>
<part name="U$8" library="power" deviceset="+3.3V" device=""/>
<part name="C7" library="ttaka_desc" deviceset="C1005" device="" value="15pF"/>
<part name="C6" library="ttaka_desc" deviceset="C1005" device="" value="750pF"/>
<part name="R1" library="ttaka_desc" deviceset="R1005" device="" value="15K"/>
<part name="U$11" library="power" deviceset="GND" device=""/>
<part name="C33" library="ttaka_desc" deviceset="C1005" device="" value="100pF"/>
<part name="R2" library="ttaka_desc" deviceset="R1005" device="" value="10K"/>
<part name="U$12" library="power" deviceset="+3.3V" device=""/>
<part name="U3" library="ttaka_ic" deviceset="CY7C6813A" device="" value="EZ-USB FX2LP CY7C6813A-56TXC"/>
<part name="U$10" library="ttaka_power" deviceset="+3.3V" device=""/>
<part name="U$18" library="ttaka_power" deviceset="GND" device=""/>
<part name="U5" library="ttaka_misc" deviceset="LDO_SOT" device="" value="LDO 3.3V 500mA SOT23-5"/>
<part name="U$21" library="ttaka_power" deviceset="GND" device=""/>
<part name="U$24" library="ttaka_power" deviceset="+3.3V" device=""/>
<part name="C23" library="ttaka_desc" deviceset="C1005" device="" value="0.1uF"/>
<part name="C22" library="ttaka_desc" deviceset="C3225" device="" value="10uF"/>
<part name="AR1" library="ttaka_desc" deviceset="ARRAY_REG__EXB28V" device="" value="1K x 4"/>
<part name="U$30" library="ttaka_power" deviceset="GND" device=""/>
<part name="C25" library="ttaka_desc" deviceset="C1005" device="" value="0.1uF"/>
<part name="C24" library="ttaka_desc" deviceset="C3225" device="" value="10uF"/>
<part name="U$39" library="ttaka_power" deviceset="GND" device=""/>
<part name="C26" library="ttaka_desc" deviceset="C1005" device="" value="0.1uF"/>
<part name="C27" library="ttaka_desc" deviceset="C1005" device="" value="0.1uF"/>
<part name="U$44" library="ttaka_power" deviceset="GND" device=""/>
<part name="XTAL" library="ttaka_misc" deviceset="XTAL_SMD" device="" value="24MHz SMD 12pF"/>
<part name="U$45" library="ttaka_power" deviceset="GND" device=""/>
<part name="J3" library="ttaka_conn" deviceset="MICROUSB" device="" value="USB MICRO B RECEPTACLE SMT"/>
<part name="U$34" library="ttaka_power" deviceset="GND" device=""/>
<part name="LED3" library="ttaka_desc" deviceset="LED1608" device="" value="LED1608 BLUE"/>
<part name="U$48" library="ttaka_power" deviceset="+3.3V" device=""/>
<part name="U$49" library="ttaka_power" deviceset="GND" device=""/>
<part name="C35" library="ttaka_desc" deviceset="C1005" device="" value="0.1uF"/>
<part name="C30" library="ttaka_desc" deviceset="C1005" device="" value="0.1uF"/>
<part name="C19" library="ttaka_desc" deviceset="C1005" device="" value="12pF"/>
<part name="C20" library="ttaka_desc" deviceset="C1005" device="" value="12pF"/>
<part name="LED1" library="ttaka_desc" deviceset="LED1608" device="" value="LED1608 GREEN"/>
<part name="U4" library="ttaka_ic" deviceset="AT24C256C" device="" value="AT24C128C 8-TSSOP"/>
<part name="U$6" library="ttaka_power" deviceset="GND" device=""/>
<part name="U$9" library="ttaka_power" deviceset="+3.3V" device=""/>
<part name="C21" library="ttaka_desc" deviceset="C1005" device="" value="1uF"/>
<part name="U$3" library="ttaka_power" deviceset="GND" device=""/>
<part name="U$5" library="ttaka_power" deviceset="+3.3V" device=""/>
<part name="J1" library="ttaka_conn" deviceset="PINH_2MM_2P" device="" value="PIN HEADER 2MM 2P"/>
<part name="U$26" library="ttaka_power" deviceset="GND" device=""/>
<part name="U$35" library="ttaka_power" deviceset="GND" device=""/>
<part name="U$1" library="power" deviceset="+3.3V" device=""/>
<part name="U2" library="ttaka_ic" deviceset="MAX2771" device=""/>
<part name="C9" library="ttaka_desc" deviceset="C1005" device="" value="470pF"/>
<part name="C3" library="ttaka_desc" deviceset="C1005" device="" value="470pF"/>
<part name="L3" library="ublox" deviceset="L1608" device="" value="8.4nH"/>
<part name="U$37" library="power" deviceset="+3.3V" device=""/>
<part name="U$38" library="power" deviceset="GND" device=""/>
<part name="U$41" library="power" deviceset="GND" device=""/>
<part name="C4" library="ttaka_desc" deviceset="C1005" device="" value="1.1pF"/>
<part name="U$43" library="power" deviceset="+3.3V" device=""/>
<part name="U$46" library="power" deviceset="+3.3V" device=""/>
<part name="U$47" library="power" deviceset="+3.3V" device=""/>
<part name="C11" library="ttaka_desc" deviceset="C1005" device="" value="15pF"/>
<part name="C10" library="ttaka_desc" deviceset="C1005" device="" value="750pF"/>
<part name="R3" library="ttaka_desc" deviceset="R1005" device="" value="15K"/>
<part name="U$50" library="power" deviceset="GND" device=""/>
<part name="R4" library="ttaka_desc" deviceset="R1005" device="" value="10K"/>
<part name="U$51" library="power" deviceset="+3.3V" device=""/>
<part name="C12" library="ttaka_desc" deviceset="C1005" device="" value="0.01uF"/>
<part name="J4" library="ttaka_conn" deviceset="SMA-ANG" device="" value="SMA JACK EDGE MNT"/>
<part name="U$56" library="power" deviceset="GND" device=""/>
<part name="RF_DIVIDER" library="ttaka_misc" deviceset="RF_DIV" device="" value="PD0922J5050S2HF"/>
<part name="U$57" library="power" deviceset="GND" device=""/>
<part name="C36" library="ttaka_desc" deviceset="C1005" device="" value="100pF"/>
<part name="C32" library="ttaka_desc" deviceset="C1005" device="" value="100pF"/>
<part name="C31" library="ttaka_desc" deviceset="C1005" device="" value="0.1uF"/>
<part name="C38" library="ttaka_desc" deviceset="C1005" device="" value="0.1uF"/>
<part name="RF-SHEILD-CASE" library="ttaka_misc" deviceset="BMI-S-202" device="" value="BMI-S-202-F BMI-S-202-C"/>
<part name="U$14" library="power" deviceset="GND" device=""/>
<part name="C14" library="ttaka_desc" deviceset="C1005" device="" value="0.01uF"/>
<part name="U$40" library="power" deviceset="GND" device=""/>
<part name="LED2" library="ttaka_desc" deviceset="LED1608" device="" value="LED1608 BLUE"/>
<part name="C28" library="ttaka_desc" deviceset="C1005" device="" value="0.1uF"/>
<part name="U$20" library="ttaka_power" deviceset="GND" device=""/>
<part name="C29" library="ttaka_desc" deviceset="C1005" device="" value="100pF"/>
<part name="C40" library="ttaka_desc" deviceset="C1005" device="" value="100pF"/>
<part name="U$36" library="power" deviceset="GND" device=""/>
<part name="U$42" library="power" deviceset="GND" device=""/>
<part name="C42" library="ttaka_desc" deviceset="C1005" device="" value="100pF"/>
<part name="U$52" library="power" deviceset="GND" device=""/>
<part name="U$53" library="ttaka_misc" deviceset="CPAD_2.2MM" device=""/>
<part name="U$54" library="ttaka_misc" deviceset="CPAD_2.2MM" device=""/>
<part name="U$55" library="ttaka_misc" deviceset="CPAD_2.2MM" device=""/>
<part name="U$58" library="ttaka_misc" deviceset="CPAD_2.2MM" device=""/>
<part name="C16" library="ttaka_desc" deviceset="C1005" device="" value="0.1uF"/>
<part name="C17" library="ttaka_desc" deviceset="C1005" device="" value="0.1uF"/>
<part name="U$17" library="ttaka_power" deviceset="GND" device=""/>
<part name="C18" library="ttaka_desc" deviceset="C1005" device="" value="100pF"/>
<part name="C37" library="ttaka_desc" deviceset="C1005" device="" value="0.1uF"/>
<part name="C41" library="ttaka_desc" deviceset="C1005" device="" value="100pF"/>
<part name="R5" library="ttaka_desc" deviceset="R1005" device="" value="2.2K"/>
<part name="R7" library="ttaka_desc" deviceset="R1005" device="" value="2.2K"/>
<part name="U$25" library="ttaka_power" deviceset="+3.3V" device=""/>
<part name="R6" library="ttaka_desc" deviceset="R1005" device="" value="10K"/>
<part name="R8" library="ttaka_desc" deviceset="R1005" device="" value="10K"/>
<part name="R9" library="ttaka_desc" deviceset="R1005" device="" value="100K"/>
<part name="R10" library="ttaka_desc" deviceset="R1005" device="" value="10K"/>
<part name="U$19" library="ttaka_power" deviceset="+3.3V" device=""/>
<part name="U$27" library="ttaka_power" deviceset="+3.3V" device=""/>
<part name="BIAS_T" library="ttaka_misc" deviceset="BIAS_T" device="" value="TCBT-2R5G+"/>
<part name="R11" library="ttaka_desc" deviceset="R1005" device="" value="100"/>
<part name="F1" library="ttaka_desc" deviceset="POLYSW1608" device="" value="12 V, 8 Ohm, Trip 250 mA,"/>
</parts>
<sheets>
<sheet>
<plain>
<text x="118.11" y="31.75" size="1.778" layer="97">EEPROM
ON: ENABLE
OFF: DISABLE</text>
</plain>
<instances>
<instance part="U1" gate="G$1" x="0" y="0" smashed="yes">
<attribute name="NAME" x="-1.016" y="3.81" size="1.27" layer="95"/>
<attribute name="VALUE" x="-3.302" y="-4.572" size="1.27" layer="96"/>
</instance>
<instance part="C5" gate="G$1" x="-40.64" y="2.54" smashed="yes">
<attribute name="NAME" x="-43.18" y="5.08" size="1.27" layer="95"/>
<attribute name="VALUE" x="-43.18" y="-2.54" size="1.27" layer="96"/>
</instance>
<instance part="C2" gate="G$1" x="-81.28" y="10.16" smashed="yes" rot="R180">
<attribute name="NAME" x="-78.74" y="12.7" size="1.27" layer="95"/>
<attribute name="VALUE" x="-78.74" y="7.62" size="1.27" layer="96"/>
</instance>
<instance part="L1" gate="G$1" x="-73.66" y="15.24" smashed="yes">
<attribute name="NAME" x="-78.486" y="17.272" size="1.27" layer="95"/>
<attribute name="VALUE" x="-73.66" y="17.272" size="1.27" layer="96"/>
</instance>
<instance part="U$13" gate="G$1" x="38.1" y="27.94" smashed="yes">
<attribute name="VALUE" x="35.56" y="29.21" size="1.778" layer="96"/>
</instance>
<instance part="U$15" gate="GND" x="-81.28" y="5.08" smashed="yes"/>
<instance part="U$16" gate="GND" x="38.1" y="-25.4" smashed="yes"/>
<instance part="C1" gate="G$1" x="-88.9" y="15.24" smashed="yes" rot="R90">
<attribute name="NAME" x="-91.44" y="17.78" size="1.27" layer="95" rot="R90"/>
<attribute name="VALUE" x="-86.36" y="17.78" size="1.27" layer="96" rot="R90"/>
</instance>
<instance part="U$22" gate="G$1" x="-48.26" y="27.94" smashed="yes">
<attribute name="VALUE" x="-50.8" y="29.21" size="1.778" layer="96"/>
</instance>
<instance part="U$23" gate="G$1" x="-48.26" y="-15.24" smashed="yes">
<attribute name="VALUE" x="-50.8" y="-13.97" size="1.778" layer="96"/>
</instance>
<instance part="C34" gate="G$1" x="-157.48" y="-165.1" smashed="yes">
<attribute name="NAME" x="-154.94" y="-162.56" size="1.27" layer="95"/>
<attribute name="VALUE" x="-154.94" y="-167.64" size="1.27" layer="96"/>
</instance>
<instance part="C39" gate="G$1" x="-142.24" y="-165.1" smashed="yes">
<attribute name="NAME" x="-139.7" y="-162.56" size="1.27" layer="95"/>
<attribute name="VALUE" x="-139.7" y="-167.64" size="1.27" layer="96"/>
</instance>
<instance part="U$31" gate="GND" x="-165.1" y="-172.72" smashed="yes"/>
<instance part="U$32" gate="G$1" x="-165.1" y="-154.94" smashed="yes">
<attribute name="VALUE" x="-167.64" y="-153.67" size="1.778" layer="96"/>
</instance>
<instance part="TCXO" gate="G$1" x="-76.2" y="-50.8" smashed="yes">
<attribute name="NAME" x="-80.01" y="-43.942" size="1.778" layer="95" ratio="20"/>
<attribute name="VALUE" x="-81.788" y="-59.436" size="1.778" layer="96" ratio="20"/>
</instance>
<instance part="U$7" gate="GND" x="-76.2" y="-66.04" smashed="yes"/>
<instance part="C13" gate="G$1" x="-101.6" y="-55.88" smashed="yes">
<attribute name="NAME" x="-99.06" y="-53.34" size="1.27" layer="95"/>
<attribute name="VALUE" x="-99.06" y="-58.42" size="1.27" layer="96"/>
</instance>
<instance part="C8" gate="G$1" x="-5.08" y="-48.26" smashed="yes" rot="R180">
<attribute name="NAME" x="-7.62" y="-50.8" size="1.27" layer="95" rot="R180"/>
<attribute name="VALUE" x="-7.62" y="-45.72" size="1.27" layer="96" rot="R180"/>
</instance>
<instance part="U$8" gate="G$1" x="10.16" y="-48.26" smashed="yes">
<attribute name="VALUE" x="7.62" y="-46.99" size="1.778" layer="96"/>
</instance>
<instance part="C7" gate="G$1" x="-15.24" y="-40.64" smashed="yes">
<attribute name="NAME" x="-20.32" y="-38.1" size="1.27" layer="95"/>
<attribute name="VALUE" x="-20.32" y="-43.18" size="1.27" layer="96"/>
</instance>
<instance part="C6" gate="G$1" x="-25.4" y="-40.64" smashed="yes">
<attribute name="NAME" x="-30.48" y="-38.1" size="1.27" layer="95"/>
<attribute name="VALUE" x="-33.02" y="-43.18" size="1.27" layer="96"/>
</instance>
<instance part="R1" gate="G$1" x="-20.32" y="-35.56" smashed="yes">
<attribute name="NAME" x="-22.606" y="-33.528" size="1.27" layer="95"/>
<attribute name="VALUE" x="-27.94" y="-33.528" size="1.27" layer="96"/>
</instance>
<instance part="U$11" gate="GND" x="-20.32" y="-53.34" smashed="yes"/>
<instance part="C33" gate="G$1" x="-127" y="-165.1" smashed="yes">
<attribute name="NAME" x="-124.46" y="-162.56" size="1.27" layer="95"/>
<attribute name="VALUE" x="-124.46" y="-167.64" size="1.27" layer="96"/>
</instance>
<instance part="R2" gate="G$1" x="0" y="43.18" smashed="yes" rot="R90">
<attribute name="NAME" x="-2.032" y="38.354" size="1.27" layer="95" rot="R90"/>
<attribute name="VALUE" x="-2.032" y="43.18" size="1.27" layer="96" rot="R90"/>
</instance>
<instance part="U$12" gate="G$1" x="0" y="50.8" smashed="yes">
<attribute name="VALUE" x="-2.54" y="52.07" size="1.778" layer="96"/>
</instance>
<instance part="C35" gate="G$1" x="-149.86" y="-165.1" smashed="yes">
<attribute name="NAME" x="-147.32" y="-162.56" size="1.27" layer="95"/>
<attribute name="VALUE" x="-147.32" y="-167.64" size="1.27" layer="96"/>
</instance>
<instance part="C30" gate="G$1" x="-134.62" y="-165.1" smashed="yes">
<attribute name="NAME" x="-132.08" y="-162.56" size="1.27" layer="95"/>
<attribute name="VALUE" x="-132.08" y="-167.64" size="1.27" layer="96"/>
</instance>
<instance part="U$1" gate="G$1" x="-101.6" y="-40.64" smashed="yes">
<attribute name="VALUE" x="-104.14" y="-39.37" size="1.778" layer="96"/>
</instance>
<instance part="U2" gate="G$1" x="0" y="-116.84" smashed="yes">
<attribute name="NAME" x="-1.016" y="-112.776" size="1.27" layer="95"/>
<attribute name="VALUE" x="-3.048" y="-121.92" size="1.27" layer="96"/>
</instance>
<instance part="C9" gate="G$1" x="-48.26" y="-114.3" smashed="yes">
<attribute name="NAME" x="-50.8" y="-111.76" size="1.27" layer="95"/>
<attribute name="VALUE" x="-53.34" y="-119.38" size="1.27" layer="96"/>
</instance>
<instance part="C3" gate="G$1" x="-88.9" y="-127" smashed="yes" rot="R90">
<attribute name="NAME" x="-91.44" y="-124.46" size="1.27" layer="95" rot="R90"/>
<attribute name="VALUE" x="-86.36" y="-124.46" size="1.27" layer="96" rot="R90"/>
</instance>
<instance part="L3" gate="G$1" x="-73.66" y="-127" smashed="yes">
<attribute name="NAME" x="-78.486" y="-124.968" size="1.27" layer="95"/>
<attribute name="VALUE" x="-73.66" y="-124.968" size="1.27" layer="96"/>
</instance>
<instance part="U$37" gate="G$1" x="38.1" y="-88.9" smashed="yes">
<attribute name="VALUE" x="35.56" y="-87.63" size="1.778" layer="96"/>
</instance>
<instance part="U$38" gate="GND" x="-81.28" y="-137.16" smashed="yes"/>
<instance part="U$41" gate="GND" x="38.1" y="-142.24" smashed="yes"/>
<instance part="C4" gate="G$1" x="-81.28" y="-132.08" smashed="yes">
<attribute name="NAME" x="-78.74" y="-129.54" size="1.27" layer="95"/>
<attribute name="VALUE" x="-78.74" y="-134.62" size="1.27" layer="96"/>
</instance>
<instance part="U$43" gate="G$1" x="-48.26" y="-88.9" smashed="yes">
<attribute name="VALUE" x="-50.8" y="-87.63" size="1.778" layer="96"/>
</instance>
<instance part="U$46" gate="G$1" x="-48.26" y="-132.08" smashed="yes">
<attribute name="VALUE" x="-50.8" y="-130.81" size="1.778" layer="96"/>
</instance>
<instance part="U$47" gate="G$1" x="10.16" y="-165.1" smashed="yes">
<attribute name="VALUE" x="7.62" y="-163.83" size="1.778" layer="96"/>
</instance>
<instance part="C11" gate="G$1" x="-15.24" y="-157.48" smashed="yes">
<attribute name="NAME" x="-20.32" y="-154.94" size="1.27" layer="95"/>
<attribute name="VALUE" x="-20.32" y="-160.02" size="1.27" layer="96"/>
</instance>
<instance part="C10" gate="G$1" x="-25.4" y="-157.48" smashed="yes">
<attribute name="NAME" x="-30.48" y="-154.94" size="1.27" layer="95"/>
<attribute name="VALUE" x="-33.02" y="-160.02" size="1.27" layer="96"/>
</instance>
<instance part="R3" gate="G$1" x="-20.32" y="-152.4" smashed="yes">
<attribute name="NAME" x="-22.606" y="-150.368" size="1.27" layer="95"/>
<attribute name="VALUE" x="-27.94" y="-150.368" size="1.27" layer="96"/>
</instance>
<instance part="U$50" gate="GND" x="-20.32" y="-172.72" smashed="yes"/>
<instance part="R4" gate="G$1" x="0" y="-73.66" smashed="yes" rot="R90">
<attribute name="NAME" x="-2.032" y="-78.486" size="1.27" layer="95" rot="R90"/>
<attribute name="VALUE" x="-2.032" y="-73.66" size="1.27" layer="96" rot="R90"/>
</instance>
<instance part="U$51" gate="G$1" x="0" y="-66.04" smashed="yes">
<attribute name="VALUE" x="-2.54" y="-64.77" size="1.778" layer="96"/>
</instance>
<instance part="C12" gate="G$1" x="-5.08" y="-165.1" smashed="yes" rot="R180">
<attribute name="NAME" x="-7.62" y="-167.64" size="1.27" layer="95" rot="R180"/>
<attribute name="VALUE" x="-7.62" y="-162.56" size="1.27" layer="96" rot="R180"/>
</instance>
<instance part="J4" gate="G$1" x="-203.2" y="-50.8" smashed="yes">
<attribute name="NAME" x="-209.55" y="-54.61" size="1.778" layer="95" rot="R90"/>
</instance>
<instance part="U$56" gate="GND" x="-203.2" y="-73.66" smashed="yes"/>
<instance part="RF_DIVIDER" gate="G$1" x="-144.78" y="-55.88" smashed="yes">
<attribute name="NAME" x="-151.13" y="-39.37" size="1.778" layer="95"/>
<attribute name="VALUE" x="-155.956" y="-44.45" size="1.778" layer="96"/>
</instance>
<instance part="U$57" gate="GND" x="-144.78" y="-73.66" smashed="yes"/>
<instance part="C36" gate="G$1" x="-119.38" y="-165.1" smashed="yes">
<attribute name="NAME" x="-116.84" y="-162.56" size="1.27" layer="95"/>
<attribute name="VALUE" x="-116.84" y="-167.64" size="1.27" layer="96"/>
</instance>
<instance part="C32" gate="G$1" x="-111.76" y="-165.1" smashed="yes">
<attribute name="NAME" x="-109.22" y="-162.56" size="1.27" layer="95"/>
<attribute name="VALUE" x="-109.22" y="-167.64" size="1.27" layer="96"/>
</instance>
<instance part="C31" gate="G$1" x="-96.52" y="-165.1" smashed="yes">
<attribute name="NAME" x="-93.98" y="-162.56" size="1.27" layer="95"/>
<attribute name="VALUE" x="-93.98" y="-167.64" size="1.27" layer="96"/>
</instance>
<instance part="C38" gate="G$1" x="-104.14" y="-165.1" smashed="yes">
<attribute name="NAME" x="-101.6" y="-162.56" size="1.27" layer="95"/>
<attribute name="VALUE" x="-101.6" y="-167.64" size="1.27" layer="96"/>
</instance>
<instance part="RF-SHEILD-CASE" gate="G$1" x="-88.9" y="-96.52" smashed="yes"/>
<instance part="U$14" gate="GND" x="-76.2" y="-101.6" smashed="yes"/>
<instance part="C14" gate="G$1" x="-165.1" y="-96.52" smashed="yes" rot="R180">
<attribute name="NAME" x="-167.64" y="-99.06" size="1.27" layer="95" rot="R180"/>
<attribute name="VALUE" x="-167.64" y="-93.98" size="1.27" layer="96" rot="R180"/>
</instance>
<instance part="U$40" gate="GND" x="-165.1" y="-101.6" smashed="yes"/>
<instance part="U$36" gate="GND" x="-35.56" y="-144.78" smashed="yes"/>
<instance part="U$42" gate="GND" x="-35.56" y="-27.94" smashed="yes"/>
<instance part="C42" gate="G$1" x="-88.9" y="-165.1" smashed="yes">
<attribute name="NAME" x="-86.36" y="-162.56" size="1.27" layer="95"/>
<attribute name="VALUE" x="-86.36" y="-167.64" size="1.27" layer="96"/>
</instance>
<instance part="U$52" gate="GND" x="5.08" y="-53.34" smashed="yes"/>
<instance part="U3" gate="G$1" x="246.38" y="-33.02" smashed="yes">
<attribute name="NAME" x="243.84" y="-26.67" size="1.778" layer="95" ratio="20"/>
<attribute name="VALUE" x="227.33" y="-41.91" size="1.778" layer="96" ratio="20"/>
</instance>
<instance part="U$10" gate="G$1" x="246.38" y="50.8" smashed="yes">
<attribute name="VALUE" x="243.84" y="52.07" size="1.778" layer="96"/>
</instance>
<instance part="U$18" gate="GND" x="246.38" y="-106.68" smashed="yes"/>
<instance part="U5" gate="G$1" x="228.6" y="-149.86" smashed="yes">
<attribute name="NAME" x="227.33" y="-138.176" size="1.778" layer="95"/>
<attribute name="VALUE" x="214.376" y="-142.748" size="1.778" layer="96"/>
</instance>
<instance part="U$21" gate="GND" x="228.6" y="-172.72" smashed="yes"/>
<instance part="U$24" gate="G$1" x="284.48" y="-139.7" smashed="yes">
<attribute name="VALUE" x="281.94" y="-138.43" size="1.778" layer="96"/>
</instance>
<instance part="C23" gate="G$1" x="210.82" y="-157.48" smashed="yes">
<attribute name="NAME" x="213.36" y="-154.94" size="1.27" layer="95"/>
<attribute name="VALUE" x="213.36" y="-160.02" size="1.27" layer="96"/>
</instance>
<instance part="C22" gate="G$1" x="200.66" y="-157.48" smashed="yes">
<attribute name="NAME" x="203.2" y="-154.94" size="1.27" layer="95"/>
<attribute name="VALUE" x="203.2" y="-160.02" size="1.27" layer="96"/>
</instance>
<instance part="AR1" gate="G$1" x="309.88" y="-149.86" smashed="yes" rot="R90">
<attribute name="NAME" x="297.18" y="-149.86" size="1.27" layer="95"/>
<attribute name="VALUE" x="322.58" y="-151.13" size="1.27" layer="96"/>
</instance>
<instance part="U$30" gate="GND" x="205.74" y="-172.72" smashed="yes"/>
<instance part="C25" gate="G$1" x="254" y="-157.48" smashed="yes">
<attribute name="NAME" x="256.54" y="-154.94" size="1.27" layer="95"/>
<attribute name="VALUE" x="256.54" y="-160.02" size="1.27" layer="96"/>
</instance>
<instance part="C24" gate="G$1" x="246.38" y="-157.48" smashed="yes">
<attribute name="NAME" x="248.92" y="-154.94" size="1.27" layer="95"/>
<attribute name="VALUE" x="248.92" y="-160.02" size="1.27" layer="96"/>
</instance>
<instance part="U$39" gate="GND" x="261.62" y="-172.72" smashed="yes"/>
<instance part="C26" gate="G$1" x="261.62" y="-157.48" smashed="yes">
<attribute name="NAME" x="264.16" y="-154.94" size="1.27" layer="95"/>
<attribute name="VALUE" x="264.16" y="-160.02" size="1.27" layer="96"/>
</instance>
<instance part="C27" gate="G$1" x="269.24" y="-157.48" smashed="yes">
<attribute name="NAME" x="271.78" y="-154.94" size="1.27" layer="95"/>
<attribute name="VALUE" x="271.78" y="-160.02" size="1.27" layer="96"/>
</instance>
<instance part="U$44" gate="GND" x="304.8" y="-172.72" smashed="yes"/>
<instance part="XTAL" gate="G$1" x="165.1" y="-53.34" smashed="yes" rot="R270">
<attribute name="NAME" x="166.878" y="-48.768" size="1.778" layer="95"/>
<attribute name="VALUE" x="162.814" y="-66.548" size="1.778" layer="95"/>
</instance>
<instance part="U$45" gate="GND" x="147.32" y="-78.74" smashed="yes"/>
<instance part="J3" gate="G$1" x="162.814" y="-137.16" smashed="yes" rot="R180">
<attribute name="NAME" x="167.894" y="-152.4" size="1.778" layer="95" rot="R180"/>
<attribute name="VALUE" x="144.526" y="-119.38" size="1.778" layer="96"/>
</instance>
<instance part="U$34" gate="GND" x="177.8" y="-172.72" smashed="yes"/>
<instance part="LED3" gate="G$1" x="314.96" y="-157.48" smashed="yes" rot="R270">
<attribute name="NAME" x="312.42" y="-166.37" size="1.27" layer="95" rot="R90"/>
</instance>
<instance part="U$48" gate="G$1" x="304.8" y="-139.7" smashed="yes">
<attribute name="VALUE" x="302.26" y="-138.43" size="1.778" layer="96"/>
</instance>
<instance part="U$49" gate="GND" x="203.2" y="-106.68" smashed="yes"/>
<instance part="C19" gate="G$1" x="157.48" y="-43.18" smashed="yes" rot="R90">
<attribute name="NAME" x="154.94" y="-40.64" size="1.27" layer="95" rot="R90"/>
<attribute name="VALUE" x="160.02" y="-40.64" size="1.27" layer="96" rot="R90"/>
</instance>
<instance part="C20" gate="G$1" x="157.48" y="-63.5" smashed="yes" rot="R90">
<attribute name="NAME" x="154.94" y="-60.96" size="1.27" layer="95" rot="R90"/>
<attribute name="VALUE" x="160.02" y="-60.96" size="1.27" layer="96" rot="R90"/>
</instance>
<instance part="LED1" gate="G$1" x="304.8" y="-157.48" smashed="yes" rot="R270">
<attribute name="NAME" x="302.26" y="-166.37" size="1.27" layer="95" rot="R90"/>
</instance>
<instance part="U4" gate="G$1" x="152.4" y="10.16" smashed="yes">
<attribute name="NAME" x="150.622" y="10.16" size="1.778" layer="95"/>
<attribute name="VALUE" x="140.462" y="-3.81" size="1.778" layer="96"/>
</instance>
<instance part="U$6" gate="GND" x="152.4" y="-10.16" smashed="yes"/>
<instance part="U$9" gate="G$1" x="165.1" y="50.8" smashed="yes">
<attribute name="VALUE" x="162.56" y="52.07" size="1.778" layer="96"/>
</instance>
<instance part="C21" gate="G$1" x="187.96" y="-25.4" smashed="yes">
<attribute name="NAME" x="190.5" y="-22.86" size="1.27" layer="95"/>
<attribute name="VALUE" x="190.5" y="-27.94" size="1.27" layer="96"/>
</instance>
<instance part="U$3" gate="GND" x="187.96" y="-30.48" smashed="yes"/>
<instance part="U$5" gate="G$1" x="134.62" y="50.8" smashed="yes">
<attribute name="VALUE" x="132.08" y="52.07" size="1.778" layer="96"/>
</instance>
<instance part="J1" gate="G$1" x="144.78" y="35.56" smashed="yes">
<attribute name="NAME" x="143.51" y="43.18" size="1.778" layer="95" ratio="5"/>
</instance>
<instance part="U$26" gate="GND" x="147.32" y="-172.72" smashed="yes"/>
<instance part="U$35" gate="GND" x="350.52" y="-172.72" smashed="yes"/>
<instance part="LED2" gate="G$1" x="309.88" y="-157.48" smashed="yes" rot="R270">
<attribute name="NAME" x="307.34" y="-166.37" size="1.27" layer="95" rot="R90"/>
</instance>
<instance part="C28" gate="G$1" x="157.48" y="35.56" smashed="yes">
<attribute name="NAME" x="160.02" y="38.1" size="1.27" layer="95"/>
<attribute name="VALUE" x="160.02" y="33.02" size="1.27" layer="96"/>
</instance>
<instance part="U$20" gate="GND" x="157.48" y="30.48" smashed="yes"/>
<instance part="C29" gate="G$1" x="276.86" y="-157.48" smashed="yes">
<attribute name="NAME" x="279.4" y="-154.94" size="1.27" layer="95"/>
<attribute name="VALUE" x="279.4" y="-160.02" size="1.27" layer="96"/>
</instance>
<instance part="C40" gate="G$1" x="284.48" y="-157.48" smashed="yes">
<attribute name="NAME" x="287.02" y="-154.94" size="1.27" layer="95"/>
<attribute name="VALUE" x="287.02" y="-160.02" size="1.27" layer="96"/>
</instance>
<instance part="U$53" gate="G$1" x="340.36" y="-152.4" smashed="yes" rot="R90"/>
<instance part="U$54" gate="G$1" x="347.98" y="-152.4" smashed="yes" rot="R90"/>
<instance part="U$55" gate="G$1" x="355.6" y="-152.4" smashed="yes" rot="R90"/>
<instance part="U$58" gate="G$1" x="363.22" y="-152.4" smashed="yes" rot="R90"/>
<instance part="C16" gate="G$1" x="-81.28" y="-165.1" smashed="yes">
<attribute name="NAME" x="-78.74" y="-162.56" size="1.27" layer="95"/>
<attribute name="VALUE" x="-78.74" y="-167.64" size="1.27" layer="96"/>
</instance>
<instance part="C17" gate="G$1" x="-73.66" y="-165.1" smashed="yes">
<attribute name="NAME" x="-71.12" y="-162.56" size="1.27" layer="95"/>
<attribute name="VALUE" x="-71.12" y="-167.64" size="1.27" layer="96"/>
</instance>
<instance part="U$17" gate="GND" x="297.18" y="-2.54" smashed="yes"/>
<instance part="C18" gate="G$1" x="-66.04" y="-165.1" smashed="yes">
<attribute name="NAME" x="-63.5" y="-162.56" size="1.27" layer="95"/>
<attribute name="VALUE" x="-63.5" y="-167.64" size="1.27" layer="96"/>
</instance>
<instance part="C37" gate="G$1" x="-58.42" y="-165.1" smashed="yes">
<attribute name="NAME" x="-55.88" y="-162.56" size="1.27" layer="95"/>
<attribute name="VALUE" x="-55.88" y="-167.64" size="1.27" layer="96"/>
</instance>
<instance part="C41" gate="G$1" x="-50.8" y="-165.1" smashed="yes">
<attribute name="NAME" x="-48.26" y="-162.56" size="1.27" layer="95"/>
<attribute name="VALUE" x="-48.26" y="-167.64" size="1.27" layer="96"/>
</instance>
<instance part="R5" gate="G$1" x="175.26" y="35.56" smashed="yes" rot="R90">
<attribute name="NAME" x="173.228" y="30.734" size="1.27" layer="95" rot="R90"/>
<attribute name="VALUE" x="173.228" y="35.56" size="1.27" layer="96" rot="R90"/>
</instance>
<instance part="R7" gate="G$1" x="180.34" y="35.56" smashed="yes" rot="R90">
<attribute name="NAME" x="178.308" y="30.734" size="1.27" layer="95" rot="R90"/>
<attribute name="VALUE" x="178.308" y="35.56" size="1.27" layer="96" rot="R90"/>
</instance>
<instance part="U$25" gate="G$1" x="175.26" y="50.8" smashed="yes">
<attribute name="VALUE" x="172.72" y="52.07" size="1.778" layer="96"/>
</instance>
<instance part="R6" gate="G$1" x="203.2" y="35.56" smashed="yes" rot="R90">
<attribute name="NAME" x="201.168" y="30.734" size="1.27" layer="95" rot="R90"/>
<attribute name="VALUE" x="201.168" y="35.56" size="1.27" layer="96" rot="R90"/>
</instance>
<instance part="R8" gate="G$1" x="198.12" y="35.56" smashed="yes" rot="R90">
<attribute name="NAME" x="196.088" y="30.734" size="1.27" layer="95" rot="R90"/>
<attribute name="VALUE" x="196.088" y="35.56" size="1.27" layer="96" rot="R90"/>
</instance>
<instance part="R9" gate="G$1" x="187.96" y="35.56" smashed="yes" rot="R90">
<attribute name="NAME" x="185.928" y="30.734" size="1.27" layer="95" rot="R90"/>
<attribute name="VALUE" x="185.928" y="35.56" size="1.27" layer="96" rot="R90"/>
</instance>
<instance part="R10" gate="G$1" x="289.56" y="35.56" smashed="yes" rot="R90">
<attribute name="NAME" x="287.528" y="30.734" size="1.27" layer="95" rot="R90"/>
<attribute name="VALUE" x="287.528" y="35.56" size="1.27" layer="96" rot="R90"/>
</instance>
<instance part="U$19" gate="G$1" x="198.12" y="50.8" smashed="yes">
<attribute name="VALUE" x="195.58" y="52.07" size="1.778" layer="96"/>
</instance>
<instance part="U$27" gate="G$1" x="289.56" y="50.8" smashed="yes">
<attribute name="VALUE" x="287.02" y="52.07" size="1.778" layer="96"/>
</instance>
<instance part="BIAS_T" gate="G$1" x="-180.34" y="-55.88" smashed="yes" rot="R180">
<attribute name="NAME" x="-183.896" y="-39.116" size="1.778" layer="95"/>
<attribute name="VALUE" x="-187.198" y="-44.196" size="1.778" layer="96"/>
</instance>
<instance part="R11" gate="G$1" x="-121.92" y="-55.88" smashed="yes" rot="R90">
<attribute name="NAME" x="-123.952" y="-60.706" size="1.27" layer="95" rot="R90"/>
<attribute name="VALUE" x="-123.952" y="-55.88" size="1.27" layer="96" rot="R90"/>
</instance>
<instance part="F1" gate="G$1" x="-144.78" y="-88.9" smashed="yes">
<attribute name="NAME" x="-149.86" y="-83.82" size="1.778" layer="95"/>
<attribute name="VALUE" x="-149.86" y="-93.98" size="1.778" layer="96"/>
</instance>
</instances>
<busses>
</busses>
<nets>
<net name="N$1" class="0">
<segment>
<pinref part="U1" gate="G$1" pin="MIXIN_HI"/>
<pinref part="C5" gate="G$1" pin="2"/>
<wire x1="-33.02" y1="0" x2="-40.64" y2="0" width="0.1524" layer="91"/>
</segment>
</net>
<net name="N$2" class="0">
<segment>
<pinref part="U1" gate="G$1" pin="LNAOUT_HI"/>
<pinref part="C5" gate="G$1" pin="1"/>
<wire x1="-33.02" y1="5.08" x2="-40.64" y2="5.08" width="0.1524" layer="91"/>
</segment>
</net>
<net name="GND" class="0">
<segment>
<pinref part="U1" gate="G$1" pin="EP"/>
<pinref part="U$16" gate="GND" pin="GND"/>
<wire x1="33.02" y1="-22.86" x2="38.1" y2="-22.86" width="0.1524" layer="91"/>
<wire x1="38.1" y1="-22.86" x2="38.1" y2="-25.4" width="0.1524" layer="91"/>
</segment>
<segment>
<wire x1="-134.62" y1="-167.64" x2="-134.62" y2="-170.18" width="0.1524" layer="91"/>
<pinref part="U$31" gate="GND" pin="GND"/>
<wire x1="-134.62" y1="-170.18" x2="-142.24" y2="-170.18" width="0.1524" layer="91"/>
<wire x1="-142.24" y1="-170.18" x2="-149.86" y2="-170.18" width="0.1524" layer="91"/>
<wire x1="-149.86" y1="-170.18" x2="-157.48" y2="-170.18" width="0.1524" layer="91"/>
<wire x1="-157.48" y1="-170.18" x2="-165.1" y2="-170.18" width="0.1524" layer="91"/>
<wire x1="-165.1" y1="-170.18" x2="-165.1" y2="-172.72" width="0.1524" layer="91"/>
<pinref part="C39" gate="G$1" pin="2"/>
<wire x1="-142.24" y1="-167.64" x2="-142.24" y2="-170.18" width="0.1524" layer="91"/>
<junction x="-142.24" y="-170.18"/>
<wire x1="-149.86" y1="-167.64" x2="-149.86" y2="-170.18" width="0.1524" layer="91"/>
<junction x="-149.86" y="-170.18"/>
<pinref part="C34" gate="G$1" pin="2"/>
<wire x1="-157.48" y1="-167.64" x2="-157.48" y2="-170.18" width="0.1524" layer="91"/>
<junction x="-157.48" y="-170.18"/>
<junction x="-134.62" y="-170.18"/>
<pinref part="C33" gate="G$1" pin="2"/>
<wire x1="-134.62" y1="-170.18" x2="-127" y2="-170.18" width="0.1524" layer="91"/>
<wire x1="-127" y1="-170.18" x2="-127" y2="-167.64" width="0.1524" layer="91"/>
<pinref part="C35" gate="G$1" pin="2"/>
<pinref part="C30" gate="G$1" pin="2"/>
<wire x1="-104.14" y1="-167.64" x2="-104.14" y2="-170.18" width="0.1524" layer="91"/>
<wire x1="-104.14" y1="-170.18" x2="-111.76" y2="-170.18" width="0.1524" layer="91"/>
<wire x1="-111.76" y1="-170.18" x2="-119.38" y2="-170.18" width="0.1524" layer="91"/>
<pinref part="C32" gate="G$1" pin="2"/>
<wire x1="-111.76" y1="-167.64" x2="-111.76" y2="-170.18" width="0.1524" layer="91"/>
<junction x="-111.76" y="-170.18"/>
<pinref part="C36" gate="G$1" pin="2"/>
<wire x1="-119.38" y1="-167.64" x2="-119.38" y2="-170.18" width="0.1524" layer="91"/>
<junction x="-119.38" y="-170.18"/>
<junction x="-104.14" y="-170.18"/>
<pinref part="C31" gate="G$1" pin="2"/>
<wire x1="-104.14" y1="-170.18" x2="-96.52" y2="-170.18" width="0.1524" layer="91"/>
<wire x1="-96.52" y1="-170.18" x2="-96.52" y2="-167.64" width="0.1524" layer="91"/>
<pinref part="C38" gate="G$1" pin="2"/>
<wire x1="-127" y1="-170.18" x2="-119.38" y2="-170.18" width="0.1524" layer="91"/>
<junction x="-127" y="-170.18"/>
<pinref part="C42" gate="G$1" pin="2"/>
<wire x1="-96.52" y1="-170.18" x2="-88.9" y2="-170.18" width="0.1524" layer="91"/>
<wire x1="-88.9" y1="-170.18" x2="-88.9" y2="-167.64" width="0.1524" layer="91"/>
<junction x="-96.52" y="-170.18"/>
<pinref part="C16" gate="G$1" pin="2"/>
<wire x1="-88.9" y1="-170.18" x2="-81.28" y2="-170.18" width="0.1524" layer="91"/>
<wire x1="-81.28" y1="-170.18" x2="-81.28" y2="-167.64" width="0.1524" layer="91"/>
<junction x="-88.9" y="-170.18"/>
<pinref part="C17" gate="G$1" pin="2"/>
<wire x1="-81.28" y1="-170.18" x2="-73.66" y2="-170.18" width="0.1524" layer="91"/>
<wire x1="-73.66" y1="-170.18" x2="-73.66" y2="-167.64" width="0.1524" layer="91"/>
<junction x="-81.28" y="-170.18"/>
<pinref part="C18" gate="G$1" pin="2"/>
<wire x1="-73.66" y1="-170.18" x2="-66.04" y2="-170.18" width="0.1524" layer="91"/>
<wire x1="-66.04" y1="-170.18" x2="-66.04" y2="-167.64" width="0.1524" layer="91"/>
<junction x="-73.66" y="-170.18"/>
<pinref part="C37" gate="G$1" pin="2"/>
<wire x1="-66.04" y1="-170.18" x2="-58.42" y2="-170.18" width="0.1524" layer="91"/>
<wire x1="-58.42" y1="-170.18" x2="-58.42" y2="-167.64" width="0.1524" layer="91"/>
<junction x="-66.04" y="-170.18"/>
<pinref part="C41" gate="G$1" pin="2"/>
<wire x1="-58.42" y1="-170.18" x2="-50.8" y2="-170.18" width="0.1524" layer="91"/>
<wire x1="-50.8" y1="-170.18" x2="-50.8" y2="-167.64" width="0.1524" layer="91"/>
<junction x="-58.42" y="-170.18"/>
</segment>
<segment>
<wire x1="-91.44" y1="-53.34" x2="-91.44" y2="-63.5" width="0.1524" layer="91"/>
<pinref part="TCXO" gate="G$1" pin="GND"/>
<wire x1="-86.36" y1="-53.34" x2="-91.44" y2="-53.34" width="0.1524" layer="91"/>
<wire x1="-60.96" y1="-63.5" x2="-60.96" y2="-53.34" width="0.1524" layer="91"/>
<pinref part="TCXO" gate="G$1" pin="NC"/>
<wire x1="-60.96" y1="-53.34" x2="-66.04" y2="-53.34" width="0.1524" layer="91"/>
<wire x1="-91.44" y1="-63.5" x2="-76.2" y2="-63.5" width="0.1524" layer="91"/>
<pinref part="U$7" gate="GND" pin="GND"/>
<wire x1="-76.2" y1="-63.5" x2="-60.96" y2="-63.5" width="0.1524" layer="91"/>
<wire x1="-76.2" y1="-63.5" x2="-76.2" y2="-66.04" width="0.1524" layer="91"/>
<junction x="-76.2" y="-63.5"/>
<wire x1="-91.44" y1="-63.5" x2="-101.6" y2="-63.5" width="0.1524" layer="91"/>
<junction x="-91.44" y="-63.5"/>
<pinref part="C13" gate="G$1" pin="2"/>
<wire x1="-101.6" y1="-63.5" x2="-101.6" y2="-58.42" width="0.1524" layer="91"/>
</segment>
<segment>
<pinref part="C6" gate="G$1" pin="2"/>
<wire x1="-25.4" y1="-43.18" x2="-25.4" y2="-48.26" width="0.1524" layer="91"/>
<pinref part="C7" gate="G$1" pin="2"/>
<wire x1="-15.24" y1="-48.26" x2="-15.24" y2="-43.18" width="0.1524" layer="91"/>
<wire x1="-25.4" y1="-48.26" x2="-20.32" y2="-48.26" width="0.1524" layer="91"/>
<pinref part="U$11" gate="GND" pin="GND"/>
<wire x1="-20.32" y1="-48.26" x2="-15.24" y2="-48.26" width="0.1524" layer="91"/>
<wire x1="-20.32" y1="-48.26" x2="-20.32" y2="-53.34" width="0.1524" layer="91"/>
<junction x="-20.32" y="-48.26"/>
</segment>
<segment>
<pinref part="C2" gate="G$1" pin="1"/>
<pinref part="U$15" gate="GND" pin="GND"/>
<wire x1="-81.28" y1="7.62" x2="-81.28" y2="5.08" width="0.1524" layer="91"/>
</segment>
<segment>
<pinref part="U2" gate="G$1" pin="EP"/>
<pinref part="U$41" gate="GND" pin="GND"/>
<wire x1="33.02" y1="-139.7" x2="38.1" y2="-139.7" width="0.1524" layer="91"/>
<wire x1="38.1" y1="-139.7" x2="38.1" y2="-142.24" width="0.1524" layer="91"/>
</segment>
<segment>
<pinref part="C4" gate="G$1" pin="2"/>
<pinref part="U$38" gate="GND" pin="GND"/>
<wire x1="-81.28" y1="-134.62" x2="-81.28" y2="-137.16" width="0.1524" layer="91"/>
</segment>
<segment>
<pinref part="C10" gate="G$1" pin="2"/>
<wire x1="-25.4" y1="-160.02" x2="-25.4" y2="-165.1" width="0.1524" layer="91"/>
<pinref part="C11" gate="G$1" pin="2"/>
<wire x1="-15.24" y1="-165.1" x2="-15.24" y2="-160.02" width="0.1524" layer="91"/>
<wire x1="-25.4" y1="-165.1" x2="-20.32" y2="-165.1" width="0.1524" layer="91"/>
<pinref part="U$50" gate="GND" pin="GND"/>
<wire x1="-20.32" y1="-165.1" x2="-15.24" y2="-165.1" width="0.1524" layer="91"/>
<wire x1="-20.32" y1="-165.1" x2="-20.32" y2="-172.72" width="0.1524" layer="91"/>
<junction x="-20.32" y="-165.1"/>
</segment>
<segment>
<pinref part="J4" gate="G$1" pin="GND"/>
<wire x1="-203.2" y1="-58.42" x2="-203.2" y2="-60.96" width="0.1524" layer="91"/>
<pinref part="U$56" gate="GND" pin="GND"/>
<pinref part="BIAS_T" gate="G$1" pin="ISOLATE"/>
<wire x1="-203.2" y1="-60.96" x2="-203.2" y2="-73.66" width="0.1524" layer="91"/>
<wire x1="-193.04" y1="-60.96" x2="-203.2" y2="-60.96" width="0.1524" layer="91"/>
<junction x="-203.2" y="-60.96"/>
</segment>
<segment>
<pinref part="RF_DIVIDER" gate="G$1" pin="GND@1"/>
<wire x1="-160.02" y1="-60.96" x2="-160.02" y2="-50.8" width="0.1524" layer="91"/>
<wire x1="-160.02" y1="-50.8" x2="-157.48" y2="-50.8" width="0.1524" layer="91"/>
<pinref part="RF_DIVIDER" gate="G$1" pin="GND@2"/>
<wire x1="-157.48" y1="-60.96" x2="-160.02" y2="-60.96" width="0.1524" layer="91"/>
<junction x="-160.02" y="-60.96"/>
<pinref part="RF_DIVIDER" gate="G$1" pin="GND@3"/>
<wire x1="-132.08" y1="-55.88" x2="-129.54" y2="-55.88" width="0.1524" layer="91"/>
<wire x1="-129.54" y1="-55.88" x2="-129.54" y2="-71.12" width="0.1524" layer="91"/>
<wire x1="-129.54" y1="-71.12" x2="-144.78" y2="-71.12" width="0.1524" layer="91"/>
<wire x1="-144.78" y1="-71.12" x2="-160.02" y2="-71.12" width="0.1524" layer="91"/>
<wire x1="-160.02" y1="-71.12" x2="-160.02" y2="-60.96" width="0.1524" layer="91"/>
<pinref part="U$57" gate="GND" pin="GND"/>
<wire x1="-144.78" y1="-71.12" x2="-144.78" y2="-73.66" width="0.1524" layer="91"/>
<junction x="-144.78" y="-71.12"/>
</segment>
<segment>
<pinref part="RF-SHEILD-CASE" gate="G$1" pin="GND"/>
<wire x1="-78.74" y1="-96.52" x2="-76.2" y2="-96.52" width="0.1524" layer="91"/>
<pinref part="U$14" gate="GND" pin="GND"/>
<wire x1="-76.2" y1="-96.52" x2="-76.2" y2="-101.6" width="0.1524" layer="91"/>
</segment>
<segment>
<pinref part="C14" gate="G$1" pin="1"/>
<pinref part="U$40" gate="GND" pin="GND"/>
<wire x1="-165.1" y1="-99.06" x2="-165.1" y2="-101.6" width="0.1524" layer="91"/>
</segment>
<segment>
<pinref part="U2" gate="G$1" pin="MIXIN_HI"/>
<pinref part="U$36" gate="GND" pin="GND"/>
<wire x1="-33.02" y1="-116.84" x2="-35.56" y2="-116.84" width="0.1524" layer="91"/>
<wire x1="-35.56" y1="-116.84" x2="-35.56" y2="-144.78" width="0.1524" layer="91"/>
<wire x1="-35.56" y1="-116.84" x2="-35.56" y2="-81.28" width="0.1524" layer="91"/>
<junction x="-35.56" y="-116.84"/>
<pinref part="U2" gate="G$1" pin="LNAIN_HI"/>
<wire x1="-35.56" y1="-81.28" x2="-5.08" y2="-81.28" width="0.1524" layer="91"/>
<wire x1="-5.08" y1="-81.28" x2="-5.08" y2="-83.82" width="0.1524" layer="91"/>
</segment>
<segment>
<pinref part="U$42" gate="GND" pin="GND"/>
<wire x1="-35.56" y1="-27.94" x2="-35.56" y2="-5.08" width="0.1524" layer="91"/>
<pinref part="U1" gate="G$1" pin="MIXIN_LO"/>
<wire x1="-35.56" y1="-5.08" x2="-33.02" y2="-5.08" width="0.1524" layer="91"/>
<wire x1="-35.56" y1="-5.08" x2="-35.56" y2="38.1" width="0.1524" layer="91"/>
<wire x1="-35.56" y1="38.1" x2="-10.16" y2="38.1" width="0.1524" layer="91"/>
<junction x="-35.56" y="-5.08"/>
<pinref part="U1" gate="G$1" pin="LNAIN_LO"/>
<wire x1="-10.16" y1="38.1" x2="-10.16" y2="33.02" width="0.1524" layer="91"/>
</segment>
<segment>
<pinref part="U1" gate="G$1" pin="ADC_CLKIN"/>
<pinref part="U$52" gate="GND" pin="GND"/>
<wire x1="5.08" y1="-33.02" x2="5.08" y2="-53.34" width="0.1524" layer="91"/>
</segment>
<segment>
<pinref part="U3" gate="G$1" pin="GND@1"/>
<wire x1="228.6" y1="-96.52" x2="228.6" y2="-101.6" width="0.1524" layer="91"/>
<wire x1="228.6" y1="-101.6" x2="233.68" y2="-101.6" width="0.1524" layer="91"/>
<pinref part="U3" gate="G$1" pin="AGND@2"/>
<wire x1="233.68" y1="-101.6" x2="238.76" y2="-101.6" width="0.1524" layer="91"/>
<wire x1="238.76" y1="-101.6" x2="243.84" y2="-101.6" width="0.1524" layer="91"/>
<wire x1="243.84" y1="-101.6" x2="246.38" y2="-101.6" width="0.1524" layer="91"/>
<wire x1="246.38" y1="-101.6" x2="248.92" y2="-101.6" width="0.1524" layer="91"/>
<wire x1="248.92" y1="-101.6" x2="254" y2="-101.6" width="0.1524" layer="91"/>
<wire x1="254" y1="-101.6" x2="259.08" y2="-101.6" width="0.1524" layer="91"/>
<wire x1="259.08" y1="-101.6" x2="264.16" y2="-101.6" width="0.1524" layer="91"/>
<wire x1="264.16" y1="-101.6" x2="264.16" y2="-96.52" width="0.1524" layer="91"/>
<pinref part="U3" gate="G$1" pin="AGND@1"/>
<wire x1="259.08" y1="-96.52" x2="259.08" y2="-101.6" width="0.1524" layer="91"/>
<junction x="259.08" y="-101.6"/>
<pinref part="U3" gate="G$1" pin="GND@6"/>
<wire x1="254" y1="-96.52" x2="254" y2="-101.6" width="0.1524" layer="91"/>
<junction x="254" y="-101.6"/>
<pinref part="U3" gate="G$1" pin="GND@5"/>
<wire x1="248.92" y1="-96.52" x2="248.92" y2="-101.6" width="0.1524" layer="91"/>
<junction x="248.92" y="-101.6"/>
<pinref part="U3" gate="G$1" pin="GND@4"/>
<wire x1="243.84" y1="-96.52" x2="243.84" y2="-101.6" width="0.1524" layer="91"/>
<junction x="243.84" y="-101.6"/>
<pinref part="U3" gate="G$1" pin="GND@3"/>
<wire x1="238.76" y1="-96.52" x2="238.76" y2="-101.6" width="0.1524" layer="91"/>
<junction x="238.76" y="-101.6"/>
<pinref part="U3" gate="G$1" pin="GND@2"/>
<wire x1="233.68" y1="-96.52" x2="233.68" y2="-101.6" width="0.1524" layer="91"/>
<junction x="233.68" y="-101.6"/>
<pinref part="U$18" gate="GND" pin="GND"/>
<wire x1="246.38" y1="-101.6" x2="246.38" y2="-106.68" width="0.1524" layer="91"/>
<junction x="246.38" y="-101.6"/>
</segment>
<segment>
<pinref part="U5" gate="G$1" pin="GND"/>
<pinref part="U$21" gate="GND" pin="GND"/>
<wire x1="228.6" y1="-157.48" x2="228.6" y2="-172.72" width="0.1524" layer="91"/>
</segment>
<segment>
<pinref part="C24" gate="G$1" pin="2"/>
<wire x1="246.38" y1="-160.02" x2="246.38" y2="-167.64" width="0.1524" layer="91"/>
<wire x1="246.38" y1="-167.64" x2="254" y2="-167.64" width="0.1524" layer="91"/>
<wire x1="254" y1="-167.64" x2="261.62" y2="-167.64" width="0.1524" layer="91"/>
<wire x1="261.62" y1="-167.64" x2="269.24" y2="-167.64" width="0.1524" layer="91"/>
<pinref part="C25" gate="G$1" pin="2"/>
<wire x1="254" y1="-160.02" x2="254" y2="-167.64" width="0.1524" layer="91"/>
<junction x="254" y="-167.64"/>
<pinref part="C26" gate="G$1" pin="2"/>
<wire x1="261.62" y1="-160.02" x2="261.62" y2="-167.64" width="0.1524" layer="91"/>
<junction x="261.62" y="-167.64"/>
<pinref part="C27" gate="G$1" pin="2"/>
<wire x1="269.24" y1="-160.02" x2="269.24" y2="-167.64" width="0.1524" layer="91"/>
<pinref part="U$39" gate="GND" pin="GND"/>
<wire x1="261.62" y1="-167.64" x2="261.62" y2="-172.72" width="0.1524" layer="91"/>
<junction x="261.62" y="-167.64"/>
<pinref part="C29" gate="G$1" pin="2"/>
<wire x1="269.24" y1="-167.64" x2="276.86" y2="-167.64" width="0.1524" layer="91"/>
<wire x1="276.86" y1="-167.64" x2="276.86" y2="-160.02" width="0.1524" layer="91"/>
<junction x="269.24" y="-167.64"/>
<pinref part="C40" gate="G$1" pin="2"/>
<wire x1="276.86" y1="-167.64" x2="284.48" y2="-167.64" width="0.1524" layer="91"/>
<wire x1="284.48" y1="-167.64" x2="284.48" y2="-160.02" width="0.1524" layer="91"/>
<junction x="276.86" y="-167.64"/>
</segment>
<segment>
<pinref part="XTAL" gate="G$1" pin="GND"/>
<pinref part="U$45" gate="GND" pin="GND"/>
<wire x1="160.02" y1="-53.34" x2="147.32" y2="-53.34" width="0.1524" layer="91"/>
<wire x1="147.32" y1="-53.34" x2="147.32" y2="-63.5" width="0.1524" layer="91"/>
<wire x1="147.32" y1="-63.5" x2="147.32" y2="-78.74" width="0.1524" layer="91"/>
<wire x1="154.94" y1="-43.18" x2="147.32" y2="-43.18" width="0.1524" layer="91"/>
<wire x1="147.32" y1="-43.18" x2="147.32" y2="-53.34" width="0.1524" layer="91"/>
<junction x="147.32" y="-53.34"/>
<wire x1="154.94" y1="-63.5" x2="147.32" y2="-63.5" width="0.1524" layer="91"/>
<junction x="147.32" y="-63.5"/>
<pinref part="C19" gate="G$1" pin="1"/>
<pinref part="C20" gate="G$1" pin="1"/>
</segment>
<segment>
<pinref part="U3" gate="G$1" pin="EP"/>
<wire x1="208.28" y1="-83.82" x2="203.2" y2="-83.82" width="0.1524" layer="91"/>
<pinref part="U$49" gate="GND" pin="GND"/>
<wire x1="203.2" y1="-83.82" x2="203.2" y2="-106.68" width="0.1524" layer="91"/>
<junction x="203.2" y="-83.82"/>
<pinref part="U3" gate="G$1" pin="RDY1/SLWR"/>
<wire x1="203.2" y1="-38.1" x2="203.2" y2="-68.58" width="0.1524" layer="91"/>
<wire x1="203.2" y1="-68.58" x2="203.2" y2="-83.82" width="0.1524" layer="91"/>
<wire x1="203.2" y1="-38.1" x2="208.28" y2="-38.1" width="0.1524" layer="91"/>
<pinref part="U3" gate="G$1" pin="RESERVED"/>
<wire x1="208.28" y1="-68.58" x2="203.2" y2="-68.58" width="0.1524" layer="91"/>
<junction x="203.2" y="-68.58"/>
</segment>
<segment>
<pinref part="LED3" gate="G$1" pin="P$2"/>
<wire x1="314.96" y1="-162.56" x2="314.96" y2="-167.64" width="0.1524" layer="91"/>
<pinref part="LED1" gate="G$1" pin="P$2"/>
<wire x1="304.8" y1="-162.56" x2="304.8" y2="-167.64" width="0.1524" layer="91"/>
<wire x1="304.8" y1="-167.64" x2="309.88" y2="-167.64" width="0.1524" layer="91"/>
<wire x1="309.88" y1="-167.64" x2="314.96" y2="-167.64" width="0.1524" layer="91"/>
<wire x1="304.8" y1="-167.64" x2="304.8" y2="-172.72" width="0.1524" layer="91"/>
<junction x="304.8" y="-167.64"/>
<pinref part="U$44" gate="GND" pin="GND"/>
<pinref part="LED2" gate="G$1" pin="P$2"/>
<wire x1="309.88" y1="-162.56" x2="309.88" y2="-167.64" width="0.1524" layer="91"/>
<junction x="309.88" y="-167.64"/>
</segment>
<segment>
<pinref part="C22" gate="G$1" pin="2"/>
<wire x1="200.66" y1="-160.02" x2="200.66" y2="-167.64" width="0.1524" layer="91"/>
<pinref part="C23" gate="G$1" pin="2"/>
<wire x1="200.66" y1="-167.64" x2="205.74" y2="-167.64" width="0.1524" layer="91"/>
<wire x1="205.74" y1="-167.64" x2="210.82" y2="-167.64" width="0.1524" layer="91"/>
<wire x1="210.82" y1="-167.64" x2="210.82" y2="-160.02" width="0.1524" layer="91"/>
<pinref part="U$30" gate="GND" pin="GND"/>
<wire x1="205.74" y1="-167.64" x2="205.74" y2="-172.72" width="0.1524" layer="91"/>
<junction x="205.74" y="-167.64"/>
</segment>
<segment>
<pinref part="U4" gate="G$1" pin="GND"/>
<wire x1="139.7" y1="2.54" x2="134.62" y2="2.54" width="0.1524" layer="91"/>
<pinref part="U4" gate="G$1" pin="A1"/>
<wire x1="139.7" y1="12.7" x2="134.62" y2="12.7" width="0.1524" layer="91"/>
<wire x1="134.62" y1="12.7" x2="134.62" y2="7.62" width="0.1524" layer="91"/>
<pinref part="U4" gate="G$1" pin="A2"/>
<wire x1="134.62" y1="7.62" x2="134.62" y2="2.54" width="0.1524" layer="91"/>
<wire x1="139.7" y1="7.62" x2="134.62" y2="7.62" width="0.1524" layer="91"/>
<junction x="134.62" y="7.62"/>
<wire x1="134.62" y1="2.54" x2="134.62" y2="-7.62" width="0.1524" layer="91"/>
<junction x="134.62" y="2.54"/>
<wire x1="134.62" y1="-7.62" x2="152.4" y2="-7.62" width="0.1524" layer="91"/>
<pinref part="U$6" gate="GND" pin="GND"/>
<wire x1="152.4" y1="-7.62" x2="152.4" y2="-10.16" width="0.1524" layer="91"/>
<pinref part="U4" gate="G$1" pin="WP"/>
<wire x1="170.18" y1="12.7" x2="165.1" y2="12.7" width="0.1524" layer="91"/>
<wire x1="170.18" y1="12.7" x2="170.18" y2="-7.62" width="0.1524" layer="91"/>
<wire x1="170.18" y1="-7.62" x2="152.4" y2="-7.62" width="0.1524" layer="91"/>
<junction x="152.4" y="-7.62"/>
</segment>
<segment>
<pinref part="C21" gate="G$1" pin="2"/>
<pinref part="U$3" gate="GND" pin="GND"/>
<wire x1="187.96" y1="-30.48" x2="187.96" y2="-27.94" width="0.1524" layer="91"/>
</segment>
<segment>
<pinref part="J3" gate="G$1" pin="GND"/>
<pinref part="U$34" gate="GND" pin="GND"/>
<wire x1="172.894" y1="-127.08" x2="177.8" y2="-127.08" width="0.1524" layer="91"/>
<wire x1="177.8" y1="-127.08" x2="177.8" y2="-172.72" width="0.1524" layer="91"/>
</segment>
<segment>
<pinref part="J3" gate="G$1" pin="FP"/>
<pinref part="U$26" gate="GND" pin="GND"/>
<wire x1="152.654" y1="-127" x2="147.32" y2="-127" width="0.1524" layer="91"/>
<wire x1="147.32" y1="-127" x2="147.32" y2="-172.72" width="0.1524" layer="91"/>
</segment>
<segment>
<pinref part="C28" gate="G$1" pin="2"/>
<pinref part="U$20" gate="GND" pin="GND"/>
<wire x1="157.48" y1="33.02" x2="157.48" y2="30.48" width="0.1524" layer="91"/>
</segment>
<segment>
<wire x1="340.36" y1="-167.64" x2="347.98" y2="-167.64" width="0.1524" layer="91"/>
<wire x1="347.98" y1="-167.64" x2="350.52" y2="-167.64" width="0.1524" layer="91"/>
<wire x1="350.52" y1="-167.64" x2="355.6" y2="-167.64" width="0.1524" layer="91"/>
<pinref part="U$35" gate="GND" pin="GND"/>
<wire x1="355.6" y1="-167.64" x2="363.22" y2="-167.64" width="0.1524" layer="91"/>
<wire x1="350.52" y1="-167.64" x2="350.52" y2="-172.72" width="0.1524" layer="91"/>
<junction x="350.52" y="-167.64"/>
<pinref part="U$53" gate="G$1" pin="P$1"/>
<wire x1="340.36" y1="-167.64" x2="340.36" y2="-157.48" width="0.1524" layer="91"/>
<pinref part="U$54" gate="G$1" pin="P$1"/>
<wire x1="347.98" y1="-167.64" x2="347.98" y2="-157.48" width="0.1524" layer="91"/>
<junction x="347.98" y="-167.64"/>
<pinref part="U$55" gate="G$1" pin="P$1"/>
<wire x1="355.6" y1="-167.64" x2="355.6" y2="-157.48" width="0.1524" layer="91"/>
<junction x="355.6" y="-167.64"/>
<pinref part="U$58" gate="G$1" pin="P$1"/>
<wire x1="363.22" y1="-167.64" x2="363.22" y2="-157.48" width="0.1524" layer="91"/>
</segment>
<segment>
<pinref part="U3" gate="G$1" pin="PA4/FIFOADR0"/>
<pinref part="U$17" gate="GND" pin="GND"/>
<wire x1="284.48" y1="7.62" x2="297.18" y2="7.62" width="0.1524" layer="91"/>
<wire x1="297.18" y1="7.62" x2="297.18" y2="-2.54" width="0.1524" layer="91"/>
</segment>
</net>
<net name="+3.3V" class="0">
<segment>
<pinref part="U1" gate="G$1" pin="VCCADC"/>
<pinref part="U$13" gate="G$1" pin="+3.3V"/>
<wire x1="33.02" y1="15.24" x2="38.1" y2="15.24" width="0.1524" layer="91"/>
<wire x1="38.1" y1="15.24" x2="38.1" y2="25.4" width="0.1524" layer="91"/>
</segment>
<segment>
<pinref part="U1" gate="G$1" pin="VCCRF"/>
<pinref part="U$22" gate="G$1" pin="+3.3V"/>
<wire x1="-33.02" y1="15.24" x2="-48.26" y2="15.24" width="0.1524" layer="91"/>
<wire x1="-48.26" y1="15.24" x2="-48.26" y2="25.4" width="0.1524" layer="91"/>
</segment>
<segment>
<pinref part="U$23" gate="G$1" pin="+3.3V"/>
<wire x1="-48.26" y1="-17.78" x2="-48.26" y2="-22.86" width="0.1524" layer="91"/>
<pinref part="U1" gate="G$1" pin="VCCVCO"/>
<wire x1="-33.02" y1="-15.24" x2="-40.64" y2="-15.24" width="0.1524" layer="91"/>
<wire x1="-40.64" y1="-15.24" x2="-40.64" y2="-22.86" width="0.1524" layer="91"/>
<pinref part="U1" gate="G$1" pin="VCCIF"/>
<wire x1="-33.02" y1="-10.16" x2="-40.64" y2="-10.16" width="0.1524" layer="91"/>
<wire x1="-40.64" y1="-10.16" x2="-40.64" y2="-15.24" width="0.1524" layer="91"/>
<junction x="-40.64" y="-15.24"/>
<wire x1="-48.26" y1="-22.86" x2="-40.64" y2="-22.86" width="0.1524" layer="91"/>
</segment>
<segment>
<pinref part="U$32" gate="G$1" pin="+3.3V"/>
<wire x1="-165.1" y1="-157.48" x2="-165.1" y2="-160.02" width="0.1524" layer="91"/>
<wire x1="-165.1" y1="-160.02" x2="-157.48" y2="-160.02" width="0.1524" layer="91"/>
<wire x1="-157.48" y1="-160.02" x2="-149.86" y2="-160.02" width="0.1524" layer="91"/>
<wire x1="-149.86" y1="-160.02" x2="-142.24" y2="-160.02" width="0.1524" layer="91"/>
<wire x1="-142.24" y1="-160.02" x2="-134.62" y2="-160.02" width="0.1524" layer="91"/>
<wire x1="-134.62" y1="-160.02" x2="-134.62" y2="-162.56" width="0.1524" layer="91"/>
<pinref part="C34" gate="G$1" pin="1"/>
<wire x1="-157.48" y1="-162.56" x2="-157.48" y2="-160.02" width="0.1524" layer="91"/>
<junction x="-157.48" y="-160.02"/>
<wire x1="-149.86" y1="-162.56" x2="-149.86" y2="-160.02" width="0.1524" layer="91"/>
<junction x="-149.86" y="-160.02"/>
<pinref part="C39" gate="G$1" pin="1"/>
<wire x1="-142.24" y1="-162.56" x2="-142.24" y2="-160.02" width="0.1524" layer="91"/>
<junction x="-142.24" y="-160.02"/>
<junction x="-134.62" y="-160.02"/>
<wire x1="-134.62" y1="-160.02" x2="-127" y2="-160.02" width="0.1524" layer="91"/>
<pinref part="C33" gate="G$1" pin="1"/>
<wire x1="-127" y1="-160.02" x2="-127" y2="-162.56" width="0.1524" layer="91"/>
<pinref part="C35" gate="G$1" pin="1"/>
<pinref part="C30" gate="G$1" pin="1"/>
<wire x1="-119.38" y1="-160.02" x2="-111.76" y2="-160.02" width="0.1524" layer="91"/>
<wire x1="-111.76" y1="-160.02" x2="-104.14" y2="-160.02" width="0.1524" layer="91"/>
<wire x1="-104.14" y1="-160.02" x2="-104.14" y2="-162.56" width="0.1524" layer="91"/>
<pinref part="C36" gate="G$1" pin="1"/>
<wire x1="-119.38" y1="-162.56" x2="-119.38" y2="-160.02" width="0.1524" layer="91"/>
<junction x="-119.38" y="-160.02"/>
<pinref part="C32" gate="G$1" pin="1"/>
<wire x1="-111.76" y1="-162.56" x2="-111.76" y2="-160.02" width="0.1524" layer="91"/>
<junction x="-111.76" y="-160.02"/>
<junction x="-104.14" y="-160.02"/>
<wire x1="-104.14" y1="-160.02" x2="-96.52" y2="-160.02" width="0.1524" layer="91"/>
<pinref part="C31" gate="G$1" pin="1"/>
<wire x1="-96.52" y1="-160.02" x2="-96.52" y2="-162.56" width="0.1524" layer="91"/>
<pinref part="C38" gate="G$1" pin="1"/>
<wire x1="-127" y1="-160.02" x2="-119.38" y2="-160.02" width="0.1524" layer="91"/>
<junction x="-127" y="-160.02"/>
<pinref part="C42" gate="G$1" pin="1"/>
<wire x1="-96.52" y1="-160.02" x2="-88.9" y2="-160.02" width="0.1524" layer="91"/>
<wire x1="-88.9" y1="-160.02" x2="-88.9" y2="-162.56" width="0.1524" layer="91"/>
<junction x="-96.52" y="-160.02"/>
<wire x1="-88.9" y1="-160.02" x2="-81.28" y2="-160.02" width="0.1524" layer="91"/>
<junction x="-88.9" y="-160.02"/>
<pinref part="C16" gate="G$1" pin="1"/>
<wire x1="-81.28" y1="-160.02" x2="-81.28" y2="-162.56" width="0.1524" layer="91"/>
<wire x1="-81.28" y1="-160.02" x2="-73.66" y2="-160.02" width="0.1524" layer="91"/>
<junction x="-81.28" y="-160.02"/>
<pinref part="C17" gate="G$1" pin="1"/>
<wire x1="-73.66" y1="-160.02" x2="-73.66" y2="-162.56" width="0.1524" layer="91"/>
<pinref part="C18" gate="G$1" pin="1"/>
<wire x1="-73.66" y1="-160.02" x2="-66.04" y2="-160.02" width="0.1524" layer="91"/>
<wire x1="-66.04" y1="-160.02" x2="-66.04" y2="-162.56" width="0.1524" layer="91"/>
<junction x="-73.66" y="-160.02"/>
<pinref part="C37" gate="G$1" pin="1"/>
<wire x1="-66.04" y1="-160.02" x2="-58.42" y2="-160.02" width="0.1524" layer="91"/>
<wire x1="-58.42" y1="-160.02" x2="-58.42" y2="-162.56" width="0.1524" layer="91"/>
<junction x="-66.04" y="-160.02"/>
<pinref part="C41" gate="G$1" pin="1"/>
<wire x1="-58.42" y1="-160.02" x2="-50.8" y2="-160.02" width="0.1524" layer="91"/>
<wire x1="-50.8" y1="-160.02" x2="-50.8" y2="-162.56" width="0.1524" layer="91"/>
<junction x="-58.42" y="-160.02"/>
</segment>
<segment>
<pinref part="U1" gate="G$1" pin="VCCCP"/>
<wire x1="-10.16" y1="-33.02" x2="-10.16" y2="-43.18" width="0.1524" layer="91"/>
<wire x1="10.16" y1="-43.18" x2="0" y2="-43.18" width="0.1524" layer="91"/>
<pinref part="U1" gate="G$1" pin="VCCD"/>
<wire x1="0" y1="-43.18" x2="-10.16" y2="-43.18" width="0.1524" layer="91"/>
<wire x1="10.16" y1="-33.02" x2="10.16" y2="-43.18" width="0.1524" layer="91"/>
<wire x1="0" y1="-43.18" x2="0" y2="-50.8" width="0.1524" layer="91"/>
<junction x="0" y="-43.18"/>
<pinref part="U$8" gate="G$1" pin="+3.3V"/>
<wire x1="0" y1="-50.8" x2="10.16" y2="-50.8" width="0.1524" layer="91"/>
</segment>
<segment>
<pinref part="R2" gate="G$1" pin="P$2"/>
<pinref part="U$12" gate="G$1" pin="+3.3V"/>
<wire x1="0" y1="48.26" x2="0" y2="45.72" width="0.1524" layer="91"/>
</segment>
<segment>
<pinref part="C13" gate="G$1" pin="1"/>
<wire x1="-101.6" y1="-48.26" x2="-101.6" y2="-53.34" width="0.1524" layer="91"/>
<pinref part="TCXO" gate="G$1" pin="VDD"/>
<wire x1="-86.36" y1="-48.26" x2="-101.6" y2="-48.26" width="0.1524" layer="91"/>
<pinref part="U$1" gate="G$1" pin="+3.3V"/>
<wire x1="-101.6" y1="-43.18" x2="-101.6" y2="-48.26" width="0.1524" layer="91"/>
<junction x="-101.6" y="-48.26"/>
</segment>
<segment>
<pinref part="U2" gate="G$1" pin="VCCADC"/>
<pinref part="U$37" gate="G$1" pin="+3.3V"/>
<wire x1="33.02" y1="-101.6" x2="38.1" y2="-101.6" width="0.1524" layer="91"/>
<wire x1="38.1" y1="-101.6" x2="38.1" y2="-91.44" width="0.1524" layer="91"/>
</segment>
<segment>
<pinref part="U2" gate="G$1" pin="VCCRF"/>
<pinref part="U$43" gate="G$1" pin="+3.3V"/>
<wire x1="-33.02" y1="-101.6" x2="-48.26" y2="-101.6" width="0.1524" layer="91"/>
<wire x1="-48.26" y1="-101.6" x2="-48.26" y2="-91.44" width="0.1524" layer="91"/>
</segment>
<segment>
<pinref part="U$46" gate="G$1" pin="+3.3V"/>
<wire x1="-48.26" y1="-134.62" x2="-48.26" y2="-139.7" width="0.1524" layer="91"/>
<pinref part="U2" gate="G$1" pin="VCCVCO"/>
<wire x1="-33.02" y1="-132.08" x2="-40.64" y2="-132.08" width="0.1524" layer="91"/>
<wire x1="-40.64" y1="-132.08" x2="-40.64" y2="-139.7" width="0.1524" layer="91"/>
<pinref part="U2" gate="G$1" pin="VCCIF"/>
<wire x1="-33.02" y1="-127" x2="-40.64" y2="-127" width="0.1524" layer="91"/>
<wire x1="-40.64" y1="-127" x2="-40.64" y2="-132.08" width="0.1524" layer="91"/>
<junction x="-40.64" y="-132.08"/>
<wire x1="-48.26" y1="-139.7" x2="-40.64" y2="-139.7" width="0.1524" layer="91"/>
</segment>
<segment>
<pinref part="U2" gate="G$1" pin="VCCCP"/>
<wire x1="-10.16" y1="-149.86" x2="-10.16" y2="-160.02" width="0.1524" layer="91"/>
<wire x1="10.16" y1="-160.02" x2="0" y2="-160.02" width="0.1524" layer="91"/>
<pinref part="U2" gate="G$1" pin="VCCD"/>
<wire x1="0" y1="-160.02" x2="-10.16" y2="-160.02" width="0.1524" layer="91"/>
<wire x1="10.16" y1="-149.86" x2="10.16" y2="-160.02" width="0.1524" layer="91"/>
<wire x1="0" y1="-160.02" x2="0" y2="-167.64" width="0.1524" layer="91"/>
<junction x="0" y="-160.02"/>
<pinref part="U$47" gate="G$1" pin="+3.3V"/>
<wire x1="0" y1="-167.64" x2="10.16" y2="-167.64" width="0.1524" layer="91"/>
</segment>
<segment>
<pinref part="R4" gate="G$1" pin="P$2"/>
<pinref part="U$51" gate="G$1" pin="+3.3V"/>
<wire x1="0" y1="-68.58" x2="0" y2="-71.12" width="0.1524" layer="91"/>
</segment>
<segment>
<pinref part="U3" gate="G$1" pin="VCC@1"/>
<wire x1="228.6" y1="35.56" x2="228.6" y2="43.18" width="0.1524" layer="91"/>
<pinref part="U3" gate="G$1" pin="AVCC@2"/>
<wire x1="228.6" y1="43.18" x2="233.68" y2="43.18" width="0.1524" layer="91"/>
<wire x1="233.68" y1="43.18" x2="238.76" y2="43.18" width="0.1524" layer="91"/>
<wire x1="238.76" y1="43.18" x2="243.84" y2="43.18" width="0.1524" layer="91"/>
<wire x1="243.84" y1="43.18" x2="246.38" y2="43.18" width="0.1524" layer="91"/>
<wire x1="246.38" y1="43.18" x2="248.92" y2="43.18" width="0.1524" layer="91"/>
<wire x1="248.92" y1="43.18" x2="254" y2="43.18" width="0.1524" layer="91"/>
<wire x1="254" y1="43.18" x2="259.08" y2="43.18" width="0.1524" layer="91"/>
<wire x1="259.08" y1="43.18" x2="264.16" y2="43.18" width="0.1524" layer="91"/>
<wire x1="264.16" y1="43.18" x2="264.16" y2="35.56" width="0.1524" layer="91"/>
<pinref part="U3" gate="G$1" pin="AVCC@1"/>
<wire x1="259.08" y1="35.56" x2="259.08" y2="43.18" width="0.1524" layer="91"/>
<junction x="259.08" y="43.18"/>
<pinref part="U3" gate="G$1" pin="VCC@6"/>
<wire x1="254" y1="35.56" x2="254" y2="43.18" width="0.1524" layer="91"/>
<junction x="254" y="43.18"/>
<pinref part="U3" gate="G$1" pin="VCC@5"/>
<wire x1="248.92" y1="35.56" x2="248.92" y2="43.18" width="0.1524" layer="91"/>
<junction x="248.92" y="43.18"/>
<pinref part="U3" gate="G$1" pin="VCC@4"/>
<wire x1="243.84" y1="35.56" x2="243.84" y2="43.18" width="0.1524" layer="91"/>
<junction x="243.84" y="43.18"/>
<pinref part="U3" gate="G$1" pin="VCC@3"/>
<wire x1="238.76" y1="35.56" x2="238.76" y2="43.18" width="0.1524" layer="91"/>
<junction x="238.76" y="43.18"/>
<pinref part="U3" gate="G$1" pin="VCC@2"/>
<wire x1="233.68" y1="35.56" x2="233.68" y2="43.18" width="0.1524" layer="91"/>
<junction x="233.68" y="43.18"/>
<pinref part="U$10" gate="G$1" pin="+3.3V"/>
<wire x1="246.38" y1="48.26" x2="246.38" y2="43.18" width="0.1524" layer="91"/>
<junction x="246.38" y="43.18"/>
</segment>
<segment>
<pinref part="U5" gate="G$1" pin="OUT"/>
<wire x1="238.76" y1="-147.32" x2="241.3" y2="-147.32" width="0.1524" layer="91"/>
<pinref part="U$24" gate="G$1" pin="+3.3V"/>
<wire x1="241.3" y1="-147.32" x2="246.38" y2="-147.32" width="0.1524" layer="91"/>
<wire x1="246.38" y1="-147.32" x2="254" y2="-147.32" width="0.1524" layer="91"/>
<wire x1="254" y1="-147.32" x2="261.62" y2="-147.32" width="0.1524" layer="91"/>
<wire x1="261.62" y1="-147.32" x2="269.24" y2="-147.32" width="0.1524" layer="91"/>
<wire x1="269.24" y1="-147.32" x2="276.86" y2="-147.32" width="0.1524" layer="91"/>
<wire x1="276.86" y1="-147.32" x2="284.48" y2="-147.32" width="0.1524" layer="91"/>
<wire x1="284.48" y1="-147.32" x2="284.48" y2="-142.24" width="0.1524" layer="91"/>
<pinref part="C24" gate="G$1" pin="1"/>
<wire x1="246.38" y1="-147.32" x2="246.38" y2="-154.94" width="0.1524" layer="91"/>
<junction x="246.38" y="-147.32"/>
<pinref part="C25" gate="G$1" pin="1"/>
<wire x1="254" y1="-147.32" x2="254" y2="-154.94" width="0.1524" layer="91"/>
<junction x="254" y="-147.32"/>
<pinref part="C26" gate="G$1" pin="1"/>
<wire x1="261.62" y1="-147.32" x2="261.62" y2="-154.94" width="0.1524" layer="91"/>
<junction x="261.62" y="-147.32"/>
<pinref part="C27" gate="G$1" pin="1"/>
<wire x1="269.24" y1="-147.32" x2="269.24" y2="-154.94" width="0.1524" layer="91"/>
<junction x="269.24" y="-147.32"/>
<pinref part="C29" gate="G$1" pin="1"/>
<wire x1="276.86" y1="-154.94" x2="276.86" y2="-147.32" width="0.1524" layer="91"/>
<junction x="276.86" y="-147.32"/>
<pinref part="C40" gate="G$1" pin="1"/>
<wire x1="284.48" y1="-154.94" x2="284.48" y2="-147.32" width="0.1524" layer="91"/>
<junction x="284.48" y="-147.32"/>
<pinref part="U5" gate="G$1" pin="NC"/>
<wire x1="238.76" y1="-152.4" x2="241.3" y2="-152.4" width="0.1524" layer="91"/>
<wire x1="241.3" y1="-152.4" x2="241.3" y2="-147.32" width="0.1524" layer="91"/>
<junction x="241.3" y="-147.32"/>
</segment>
<segment>
<pinref part="U$9" gate="G$1" pin="+3.3V"/>
<wire x1="165.1" y1="48.26" x2="165.1" y2="43.18" width="0.1524" layer="91"/>
<pinref part="U4" gate="G$1" pin="VCC"/>
<pinref part="C28" gate="G$1" pin="1"/>
<wire x1="165.1" y1="43.18" x2="165.1" y2="17.78" width="0.1524" layer="91"/>
<wire x1="157.48" y1="38.1" x2="157.48" y2="43.18" width="0.1524" layer="91"/>
<wire x1="157.48" y1="43.18" x2="165.1" y2="43.18" width="0.1524" layer="91"/>
<junction x="165.1" y="43.18"/>
</segment>
<segment>
<pinref part="U$5" gate="G$1" pin="+3.3V"/>
<wire x1="134.62" y1="48.26" x2="134.62" y2="38.1" width="0.1524" layer="91"/>
<pinref part="J1" gate="G$1" pin="P1"/>
<wire x1="134.62" y1="38.1" x2="140.716" y2="38.1" width="0.1524" layer="91"/>
</segment>
<segment>
<pinref part="U$48" gate="G$1" pin="+3.3V"/>
<pinref part="AR1" gate="G$1" pin="P2"/>
<wire x1="304.8" y1="-147.32" x2="304.8" y2="-142.24" width="0.1524" layer="91"/>
</segment>
<segment>
<pinref part="R5" gate="G$1" pin="P$2"/>
<pinref part="U$25" gate="G$1" pin="+3.3V"/>
<wire x1="175.26" y1="38.1" x2="175.26" y2="43.18" width="0.1524" layer="91"/>
<pinref part="R7" gate="G$1" pin="P$2"/>
<wire x1="175.26" y1="43.18" x2="175.26" y2="48.26" width="0.1524" layer="91"/>
<wire x1="175.26" y1="43.18" x2="180.34" y2="43.18" width="0.1524" layer="91"/>
<wire x1="180.34" y1="43.18" x2="180.34" y2="38.1" width="0.1524" layer="91"/>
<junction x="175.26" y="43.18"/>
</segment>
<segment>
<pinref part="R10" gate="G$1" pin="P$2"/>
<pinref part="U$27" gate="G$1" pin="+3.3V"/>
<wire x1="289.56" y1="38.1" x2="289.56" y2="48.26" width="0.1524" layer="91"/>
</segment>
<segment>
<pinref part="U$19" gate="G$1" pin="+3.3V"/>
<pinref part="R8" gate="G$1" pin="P$2"/>
<wire x1="198.12" y1="48.26" x2="198.12" y2="43.18" width="0.1524" layer="91"/>
<pinref part="R6" gate="G$1" pin="P$2"/>
<wire x1="198.12" y1="43.18" x2="198.12" y2="38.1" width="0.1524" layer="91"/>
<wire x1="203.2" y1="38.1" x2="203.2" y2="43.18" width="0.1524" layer="91"/>
<junction x="198.12" y="43.18"/>
<wire x1="198.12" y1="43.18" x2="203.2" y2="43.18" width="0.1524" layer="91"/>
<wire x1="198.12" y1="43.18" x2="187.96" y2="43.18" width="0.1524" layer="91"/>
<pinref part="R9" gate="G$1" pin="P$2"/>
<wire x1="187.96" y1="43.18" x2="187.96" y2="38.1" width="0.1524" layer="91"/>
</segment>
</net>
<net name="N$24" class="0">
<segment>
<pinref part="U1" gate="G$1" pin="CPOUT"/>
<pinref part="C7" gate="G$1" pin="1"/>
<wire x1="-15.24" y1="-33.02" x2="-15.24" y2="-35.56" width="0.1524" layer="91"/>
<pinref part="R1" gate="G$1" pin="P$2"/>
<wire x1="-15.24" y1="-35.56" x2="-15.24" y2="-38.1" width="0.1524" layer="91"/>
<wire x1="-17.78" y1="-35.56" x2="-15.24" y2="-35.56" width="0.1524" layer="91"/>
<junction x="-15.24" y="-35.56"/>
</segment>
</net>
<net name="N$25" class="0">
<segment>
<pinref part="R1" gate="G$1" pin="P$1"/>
<pinref part="C6" gate="G$1" pin="1"/>
<wire x1="-22.86" y1="-35.56" x2="-25.4" y2="-35.56" width="0.1524" layer="91"/>
<wire x1="-25.4" y1="-35.56" x2="-25.4" y2="-38.1" width="0.1524" layer="91"/>
</segment>
</net>
<net name="DCLK" class="0">
<segment>
<pinref part="U1" gate="G$1" pin="CLKOUT"/>
<wire x1="0" y1="-33.02" x2="0" y2="-38.1" width="0.1524" layer="91"/>
<wire x1="0" y1="-38.1" x2="60.96" y2="-38.1" width="0.1524" layer="91"/>
<wire x1="60.96" y1="-38.1" x2="60.96" y2="-25.4" width="0.1524" layer="91"/>
<wire x1="60.96" y1="-25.4" x2="88.9" y2="-25.4" width="0.1524" layer="91"/>
<label x="91.44" y="-25.4" size="1.778" layer="95"/>
<pinref part="U2" gate="G$1" pin="ADC_CLKIN"/>
<wire x1="5.08" y1="-149.86" x2="5.08" y2="-154.94" width="0.1524" layer="91"/>
<wire x1="5.08" y1="-154.94" x2="60.96" y2="-154.94" width="0.1524" layer="91"/>
<junction x="60.96" y="-38.1"/>
<wire x1="60.96" y1="-38.1" x2="60.96" y2="-154.94" width="0.1524" layer="91"/>
</segment>
<segment>
<wire x1="198.12" y1="-63.5" x2="198.12" y2="-129.54" width="0.1524" layer="91"/>
<wire x1="198.12" y1="-129.54" x2="330.2" y2="-129.54" width="0.1524" layer="91"/>
<label x="332.74" y="-129.54" size="1.778" layer="95"/>
<pinref part="U3" gate="G$1" pin="IFCLK/PE0"/>
<wire x1="208.28" y1="-63.5" x2="198.12" y2="-63.5" width="0.1524" layer="91"/>
</segment>
</net>
<net name="N$9" class="0">
<segment>
<pinref part="C1" gate="G$1" pin="2"/>
<pinref part="L1" gate="G$1" pin="P$1"/>
<wire x1="-86.36" y1="15.24" x2="-81.28" y2="15.24" width="0.1524" layer="91"/>
<pinref part="C2" gate="G$1" pin="2"/>
<wire x1="-81.28" y1="15.24" x2="-76.2" y2="15.24" width="0.1524" layer="91"/>
<wire x1="-81.28" y1="15.24" x2="-81.28" y2="12.7" width="0.1524" layer="91"/>
<junction x="-81.28" y="15.24"/>
</segment>
</net>
<net name="N$12" class="0">
<segment>
<pinref part="L1" gate="G$1" pin="P$2"/>
<wire x1="-71.12" y1="15.24" x2="-63.5" y2="15.24" width="0.1524" layer="91"/>
<wire x1="-63.5" y1="15.24" x2="-63.5" y2="43.18" width="0.1524" layer="91"/>
<wire x1="-63.5" y1="43.18" x2="-5.08" y2="43.18" width="0.1524" layer="91"/>
<pinref part="U1" gate="G$1" pin="LNAIN_HI"/>
<wire x1="-5.08" y1="43.18" x2="-5.08" y2="33.02" width="0.1524" layer="91"/>
</segment>
</net>
<net name="N$18" class="0">
<segment>
<pinref part="U1" gate="G$1" pin="SHDN"/>
<pinref part="R2" gate="G$1" pin="P$1"/>
<wire x1="0" y1="40.64" x2="0" y2="33.02" width="0.1524" layer="91"/>
</segment>
</net>
<net name="N$15" class="0">
<segment>
<pinref part="U2" gate="G$1" pin="MIXIN_LO"/>
<pinref part="C9" gate="G$1" pin="2"/>
<wire x1="-33.02" y1="-121.92" x2="-48.26" y2="-121.92" width="0.1524" layer="91"/>
<wire x1="-48.26" y1="-121.92" x2="-48.26" y2="-116.84" width="0.1524" layer="91"/>
</segment>
</net>
<net name="N$16" class="0">
<segment>
<pinref part="C9" gate="G$1" pin="1"/>
<pinref part="U2" gate="G$1" pin="LNAOUT_LO"/>
<wire x1="-48.26" y1="-111.76" x2="-48.26" y2="-106.68" width="0.1524" layer="91"/>
<wire x1="-48.26" y1="-106.68" x2="-33.02" y2="-106.68" width="0.1524" layer="91"/>
</segment>
</net>
<net name="N$27" class="0">
<segment>
<pinref part="U2" gate="G$1" pin="CPOUT"/>
<pinref part="C11" gate="G$1" pin="1"/>
<wire x1="-15.24" y1="-149.86" x2="-15.24" y2="-152.4" width="0.1524" layer="91"/>
<pinref part="R3" gate="G$1" pin="P$2"/>
<wire x1="-15.24" y1="-152.4" x2="-15.24" y2="-154.94" width="0.1524" layer="91"/>
<wire x1="-17.78" y1="-152.4" x2="-15.24" y2="-152.4" width="0.1524" layer="91"/>
<junction x="-15.24" y="-152.4"/>
</segment>
</net>
<net name="N$32" class="0">
<segment>
<pinref part="R3" gate="G$1" pin="P$1"/>
<pinref part="C10" gate="G$1" pin="1"/>
<wire x1="-22.86" y1="-152.4" x2="-25.4" y2="-152.4" width="0.1524" layer="91"/>
<wire x1="-25.4" y1="-152.4" x2="-25.4" y2="-154.94" width="0.1524" layer="91"/>
</segment>
</net>
<net name="N$37" class="0">
<segment>
<pinref part="C3" gate="G$1" pin="2"/>
<pinref part="L3" gate="G$1" pin="P$1"/>
<wire x1="-86.36" y1="-127" x2="-81.28" y2="-127" width="0.1524" layer="91"/>
<pinref part="C4" gate="G$1" pin="1"/>
<wire x1="-81.28" y1="-127" x2="-76.2" y2="-127" width="0.1524" layer="91"/>
<wire x1="-81.28" y1="-127" x2="-81.28" y2="-129.54" width="0.1524" layer="91"/>
<junction x="-81.28" y="-127"/>
</segment>
</net>
<net name="N$38" class="0">
<segment>
<pinref part="L3" gate="G$1" pin="P$2"/>
<wire x1="-71.12" y1="-127" x2="-63.5" y2="-127" width="0.1524" layer="91"/>
<wire x1="-63.5" y1="-127" x2="-63.5" y2="-76.2" width="0.1524" layer="91"/>
<wire x1="-63.5" y1="-76.2" x2="-10.16" y2="-76.2" width="0.1524" layer="91"/>
<pinref part="U2" gate="G$1" pin="LNAIN_LO"/>
<wire x1="-10.16" y1="-76.2" x2="-10.16" y2="-83.82" width="0.1524" layer="91"/>
</segment>
</net>
<net name="N$42" class="0">
<segment>
<pinref part="U2" gate="G$1" pin="SHDN"/>
<pinref part="R4" gate="G$1" pin="P$1"/>
<wire x1="0" y1="-76.2" x2="0" y2="-83.82" width="0.1524" layer="91"/>
</segment>
</net>
<net name="N$10" class="0">
<segment>
<pinref part="U1" gate="G$1" pin="XTAL"/>
<pinref part="C8" gate="G$1" pin="2"/>
<wire x1="-5.08" y1="-33.02" x2="-5.08" y2="-45.72" width="0.1524" layer="91"/>
</segment>
</net>
<net name="N$43" class="0">
<segment>
<pinref part="U2" gate="G$1" pin="XTAL"/>
<pinref part="C12" gate="G$1" pin="2"/>
<wire x1="-5.08" y1="-149.86" x2="-5.08" y2="-162.56" width="0.1524" layer="91"/>
</segment>
</net>
<net name="PLL_CLK" class="0">
<segment>
<pinref part="TCXO" gate="G$1" pin="OUT"/>
<wire x1="-66.04" y1="-48.26" x2="-55.88" y2="-48.26" width="0.1524" layer="91"/>
<label x="-53.34" y="-48.26" size="1.778" layer="95"/>
</segment>
<segment>
<pinref part="C8" gate="G$1" pin="1"/>
<wire x1="-5.08" y1="-50.8" x2="-5.08" y2="-55.88" width="0.1524" layer="91"/>
<label x="-10.16" y="-58.42" size="1.778" layer="95"/>
</segment>
<segment>
<pinref part="C12" gate="G$1" pin="1"/>
<wire x1="-5.08" y1="-167.64" x2="-5.08" y2="-172.72" width="0.1524" layer="91"/>
<label x="-10.16" y="-175.26" size="1.778" layer="95"/>
</segment>
</net>
<net name="N$14" class="0">
<segment>
<pinref part="C1" gate="G$1" pin="1"/>
<wire x1="-91.44" y1="15.24" x2="-109.22" y2="15.24" width="0.1524" layer="91"/>
<pinref part="RF_DIVIDER" gate="G$1" pin="OUT2"/>
<wire x1="-132.08" y1="-50.8" x2="-121.92" y2="-50.8" width="0.1524" layer="91"/>
<wire x1="-121.92" y1="-50.8" x2="-109.22" y2="-50.8" width="0.1524" layer="91"/>
<wire x1="-109.22" y1="-50.8" x2="-109.22" y2="15.24" width="0.1524" layer="91"/>
<pinref part="R11" gate="G$1" pin="P$2"/>
<wire x1="-121.92" y1="-53.34" x2="-121.92" y2="-50.8" width="0.1524" layer="91"/>
<junction x="-121.92" y="-50.8"/>
</segment>
</net>
<net name="N$35" class="0">
<segment>
<pinref part="C3" gate="G$1" pin="1"/>
<pinref part="RF_DIVIDER" gate="G$1" pin="OUT1"/>
<wire x1="-109.22" y1="-127" x2="-91.44" y2="-127" width="0.1524" layer="91"/>
<wire x1="-132.08" y1="-60.96" x2="-121.92" y2="-60.96" width="0.1524" layer="91"/>
<wire x1="-121.92" y1="-60.96" x2="-109.22" y2="-60.96" width="0.1524" layer="91"/>
<wire x1="-109.22" y1="-60.96" x2="-109.22" y2="-127" width="0.1524" layer="91"/>
<pinref part="R11" gate="G$1" pin="P$1"/>
<wire x1="-121.92" y1="-58.42" x2="-121.92" y2="-60.96" width="0.1524" layer="91"/>
<junction x="-121.92" y="-60.96"/>
</segment>
</net>
<net name="I1_A" class="0">
<segment>
<pinref part="U1" gate="G$1" pin="I1"/>
<wire x1="33.02" y1="10.16" x2="88.9" y2="10.16" width="0.1524" layer="91"/>
<label x="91.44" y="10.16" size="1.778" layer="95"/>
</segment>
<segment>
<pinref part="U3" gate="G$1" pin="PB1/FD1"/>
<wire x1="284.48" y1="-17.78" x2="330.2" y2="-17.78" width="0.1524" layer="91"/>
<label x="332.74" y="-17.78" size="1.778" layer="95"/>
</segment>
</net>
<net name="I0_A" class="0">
<segment>
<pinref part="U1" gate="G$1" pin="I0"/>
<wire x1="33.02" y1="5.08" x2="88.9" y2="5.08" width="0.1524" layer="91"/>
<label x="91.44" y="5.08" size="1.778" layer="95"/>
</segment>
<segment>
<pinref part="U3" gate="G$1" pin="PB0/FD0"/>
<wire x1="284.48" y1="-12.7" x2="330.2" y2="-12.7" width="0.1524" layer="91"/>
<label x="332.74" y="-12.7" size="1.778" layer="95"/>
</segment>
</net>
<net name="Q0_A" class="0">
<segment>
<pinref part="U1" gate="G$1" pin="Q0"/>
<wire x1="33.02" y1="0" x2="88.9" y2="0" width="0.1524" layer="91"/>
<label x="91.44" y="0" size="1.778" layer="95"/>
</segment>
<segment>
<pinref part="U3" gate="G$1" pin="PB2/FD2"/>
<wire x1="284.48" y1="-22.86" x2="330.2" y2="-22.86" width="0.1524" layer="91"/>
<label x="332.74" y="-22.86" size="1.778" layer="95"/>
</segment>
</net>
<net name="Q1_A" class="0">
<segment>
<pinref part="U1" gate="G$1" pin="Q1"/>
<wire x1="33.02" y1="-5.08" x2="88.9" y2="-5.08" width="0.1524" layer="91"/>
<label x="91.44" y="-5.08" size="1.778" layer="95"/>
</segment>
<segment>
<pinref part="U3" gate="G$1" pin="PB3/FD3"/>
<wire x1="284.48" y1="-27.94" x2="330.2" y2="-27.94" width="0.1524" layer="91"/>
<label x="332.74" y="-27.94" size="1.778" layer="95"/>
</segment>
</net>
<net name="CSN_A" class="0">
<segment>
<pinref part="U1" gate="G$1" pin="CSN"/>
<wire x1="33.02" y1="-10.16" x2="88.9" y2="-10.16" width="0.1524" layer="91"/>
<label x="91.44" y="-10.16" size="1.778" layer="95"/>
</segment>
<segment>
<pinref part="U3" gate="G$1" pin="PD0/FD8"/>
<wire x1="284.48" y1="-53.34" x2="330.2" y2="-53.34" width="0.1524" layer="91"/>
<label x="332.74" y="-53.34" size="1.778" layer="95"/>
</segment>
</net>
<net name="SCLK" class="0">
<segment>
<pinref part="U1" gate="G$1" pin="SCLK"/>
<wire x1="33.02" y1="-15.24" x2="50.8" y2="-15.24" width="0.1524" layer="91"/>
<label x="91.44" y="-15.24" size="1.778" layer="95"/>
<pinref part="U2" gate="G$1" pin="SCLK"/>
<wire x1="50.8" y1="-15.24" x2="88.9" y2="-15.24" width="0.1524" layer="91"/>
<wire x1="33.02" y1="-132.08" x2="50.8" y2="-132.08" width="0.1524" layer="91"/>
<wire x1="50.8" y1="-132.08" x2="50.8" y2="-15.24" width="0.1524" layer="91"/>
<junction x="50.8" y="-15.24"/>
</segment>
<segment>
<pinref part="U3" gate="G$1" pin="PD2/FD10"/>
<wire x1="284.48" y1="-63.5" x2="330.2" y2="-63.5" width="0.1524" layer="91"/>
<label x="332.74" y="-63.5" size="1.778" layer="95"/>
</segment>
</net>
<net name="SDATA" class="0">
<segment>
<pinref part="U1" gate="G$1" pin="SDATA"/>
<wire x1="15.24" y1="-33.02" x2="55.88" y2="-33.02" width="0.1524" layer="91"/>
<wire x1="55.88" y1="-20.32" x2="88.9" y2="-20.32" width="0.1524" layer="91"/>
<label x="91.44" y="-20.32" size="1.778" layer="95"/>
<wire x1="55.88" y1="-20.32" x2="55.88" y2="-33.02" width="0.1524" layer="91"/>
<pinref part="U2" gate="G$1" pin="SDATA"/>
<wire x1="15.24" y1="-149.86" x2="55.88" y2="-149.86" width="0.1524" layer="91"/>
<junction x="55.88" y="-33.02"/>
<wire x1="55.88" y1="-33.02" x2="55.88" y2="-149.86" width="0.1524" layer="91"/>
</segment>
<segment>
<pinref part="U3" gate="G$1" pin="PD3/FD11"/>
<wire x1="284.48" y1="-68.58" x2="330.2" y2="-68.58" width="0.1524" layer="91"/>
<label x="332.74" y="-68.58" size="1.778" layer="95"/>
</segment>
</net>
<net name="I1_B" class="0">
<segment>
<pinref part="U2" gate="G$1" pin="I1"/>
<wire x1="33.02" y1="-106.68" x2="88.9" y2="-106.68" width="0.1524" layer="91"/>
<label x="91.44" y="-106.68" size="1.778" layer="95"/>
</segment>
<segment>
<pinref part="U3" gate="G$1" pin="PB5/FD5"/>
<wire x1="284.48" y1="-38.1" x2="330.2" y2="-38.1" width="0.1524" layer="91"/>
<label x="332.74" y="-38.1" size="1.778" layer="95"/>
</segment>
</net>
<net name="I0_B" class="0">
<segment>
<pinref part="U2" gate="G$1" pin="I0"/>
<wire x1="33.02" y1="-111.76" x2="88.9" y2="-111.76" width="0.1524" layer="91"/>
<label x="91.44" y="-111.76" size="1.778" layer="95"/>
</segment>
<segment>
<pinref part="U3" gate="G$1" pin="PB4/FD4"/>
<wire x1="284.48" y1="-33.02" x2="330.2" y2="-33.02" width="0.1524" layer="91"/>
<label x="332.74" y="-33.02" size="1.778" layer="95"/>
</segment>
</net>
<net name="Q0_B" class="0">
<segment>
<pinref part="U2" gate="G$1" pin="Q0"/>
<wire x1="33.02" y1="-116.84" x2="88.9" y2="-116.84" width="0.1524" layer="91"/>
<label x="91.44" y="-116.84" size="1.778" layer="95"/>
</segment>
<segment>
<pinref part="U3" gate="G$1" pin="PB6/FD6"/>
<wire x1="284.48" y1="-43.18" x2="330.2" y2="-43.18" width="0.1524" layer="91"/>
<label x="332.74" y="-43.18" size="1.778" layer="95"/>
</segment>
</net>
<net name="Q1_B" class="0">
<segment>
<pinref part="U2" gate="G$1" pin="Q1"/>
<wire x1="33.02" y1="-121.92" x2="88.9" y2="-121.92" width="0.1524" layer="91"/>
<label x="91.44" y="-121.92" size="1.778" layer="95"/>
</segment>
<segment>
<pinref part="U3" gate="G$1" pin="PB7/FD7"/>
<wire x1="284.48" y1="-48.26" x2="330.2" y2="-48.26" width="0.1524" layer="91"/>
<label x="332.74" y="-48.26" size="1.778" layer="95"/>
</segment>
</net>
<net name="CSN_B" class="0">
<segment>
<pinref part="U2" gate="G$1" pin="CSN"/>
<wire x1="33.02" y1="-127" x2="88.9" y2="-127" width="0.1524" layer="91"/>
<label x="91.44" y="-127" size="1.778" layer="95"/>
</segment>
<segment>
<pinref part="U3" gate="G$1" pin="PD1/FD9"/>
<wire x1="284.48" y1="-58.42" x2="330.2" y2="-58.42" width="0.1524" layer="91"/>
<label x="332.74" y="-58.42" size="1.778" layer="95"/>
</segment>
</net>
<net name="LOCK_A" class="0">
<segment>
<pinref part="U1" gate="G$1" pin="LD"/>
<wire x1="5.08" y1="33.02" x2="5.08" y2="38.1" width="0.1524" layer="91"/>
<wire x1="5.08" y1="38.1" x2="88.9" y2="38.1" width="0.1524" layer="91"/>
<label x="91.44" y="38.1" size="1.778" layer="95"/>
</segment>
<segment>
<pinref part="U3" gate="G$1" pin="PD4/FD12"/>
<wire x1="284.48" y1="-73.66" x2="330.2" y2="-73.66" width="0.1524" layer="91"/>
<label x="332.74" y="-73.66" size="1.778" layer="95"/>
</segment>
</net>
<net name="LOCK_B" class="0">
<segment>
<pinref part="U2" gate="G$1" pin="LD"/>
<wire x1="5.08" y1="-83.82" x2="5.08" y2="-78.74" width="0.1524" layer="91"/>
<wire x1="5.08" y1="-78.74" x2="88.9" y2="-78.74" width="0.1524" layer="91"/>
<label x="91.44" y="-78.74" size="1.778" layer="95"/>
</segment>
<segment>
<pinref part="U3" gate="G$1" pin="PD5/FD13"/>
<wire x1="284.48" y1="-78.74" x2="330.2" y2="-78.74" width="0.1524" layer="91"/>
<label x="332.74" y="-78.74" size="1.778" layer="95"/>
</segment>
</net>
<net name="N$4" class="0">
<segment>
<wire x1="-165.1" y1="-50.8" x2="-165.1" y2="-55.88" width="0.1524" layer="91"/>
<pinref part="RF_DIVIDER" gate="G$1" pin="INPUT"/>
<wire x1="-165.1" y1="-55.88" x2="-157.48" y2="-55.88" width="0.1524" layer="91"/>
<pinref part="BIAS_T" gate="G$1" pin="RF"/>
<wire x1="-167.64" y1="-50.8" x2="-165.1" y2="-50.8" width="0.1524" layer="91"/>
</segment>
</net>
<net name="N$5" class="0">
<segment>
<pinref part="C14" gate="G$1" pin="2"/>
<pinref part="BIAS_T" gate="G$1" pin="DC"/>
<wire x1="-167.64" y1="-60.96" x2="-165.1" y2="-60.96" width="0.1524" layer="91"/>
<wire x1="-165.1" y1="-60.96" x2="-165.1" y2="-88.9" width="0.1524" layer="91"/>
<pinref part="F1" gate="G$1" pin="P$1"/>
<wire x1="-165.1" y1="-88.9" x2="-165.1" y2="-93.98" width="0.1524" layer="91"/>
<wire x1="-165.1" y1="-88.9" x2="-154.94" y2="-88.9" width="0.1524" layer="91"/>
<junction x="-165.1" y="-88.9"/>
</segment>
</net>
<net name="+5V" class="0">
<segment>
<pinref part="U5" gate="G$1" pin="IN"/>
<wire x1="200.66" y1="-147.32" x2="210.82" y2="-147.32" width="0.1524" layer="91"/>
<pinref part="C22" gate="G$1" pin="1"/>
<wire x1="210.82" y1="-147.32" x2="215.9" y2="-147.32" width="0.1524" layer="91"/>
<wire x1="215.9" y1="-147.32" x2="218.44" y2="-147.32" width="0.1524" layer="91"/>
<wire x1="200.66" y1="-147.32" x2="200.66" y2="-154.94" width="0.1524" layer="91"/>
<pinref part="C23" gate="G$1" pin="1"/>
<wire x1="210.82" y1="-147.32" x2="210.82" y2="-154.94" width="0.1524" layer="91"/>
<junction x="210.82" y="-147.32"/>
<pinref part="J3" gate="G$1" pin="VBUS"/>
<wire x1="172.894" y1="-147.24" x2="180.34" y2="-147.24" width="0.1524" layer="91"/>
<wire x1="180.34" y1="-147.24" x2="200.66" y2="-147.32" width="0.1524" layer="91"/>
<junction x="200.66" y="-147.32"/>
<pinref part="U5" gate="G$1" pin="EN"/>
<wire x1="218.44" y1="-152.4" x2="215.9" y2="-152.4" width="0.1524" layer="91"/>
<wire x1="215.9" y1="-152.4" x2="215.9" y2="-147.32" width="0.1524" layer="91"/>
<junction x="215.9" y="-147.32"/>
<label x="182.88" y="-149.86" size="1.778" layer="95"/>
</segment>
<segment>
<wire x1="-134.62" y1="-88.9" x2="-119.38" y2="-88.9" width="0.1524" layer="91"/>
<label x="-119.38" y="-86.36" size="1.778" layer="95"/>
<pinref part="F1" gate="G$1" pin="P$2"/>
</segment>
</net>
<net name="N$17" class="0">
<segment>
<pinref part="U3" gate="G$1" pin="XTALOUT"/>
<wire x1="165.1" y1="-43.18" x2="208.28" y2="-43.18" width="0.1524" layer="91"/>
<pinref part="XTAL" gate="G$1" pin="XTAL1"/>
<wire x1="165.1" y1="-48.26" x2="165.1" y2="-43.18" width="0.1524" layer="91"/>
<wire x1="165.1" y1="-43.18" x2="160.02" y2="-43.18" width="0.1524" layer="91"/>
<junction x="165.1" y="-43.18"/>
<pinref part="C19" gate="G$1" pin="2"/>
</segment>
</net>
<net name="N$22" class="0">
<segment>
<pinref part="XTAL" gate="G$1" pin="XTAL2"/>
<wire x1="165.1" y1="-58.42" x2="165.1" y2="-63.5" width="0.1524" layer="91"/>
<wire x1="160.02" y1="-63.5" x2="165.1" y2="-63.5" width="0.1524" layer="91"/>
<pinref part="C20" gate="G$1" pin="2"/>
<pinref part="U3" gate="G$1" pin="XTALIN"/>
<wire x1="208.28" y1="-48.26" x2="175.26" y2="-48.26" width="0.1524" layer="91"/>
<wire x1="175.26" y1="-48.26" x2="175.26" y2="-63.5" width="0.1524" layer="91"/>
<wire x1="175.26" y1="-63.5" x2="165.1" y2="-63.5" width="0.1524" layer="91"/>
<junction x="165.1" y="-63.5"/>
</segment>
</net>
<net name="N$13" class="0">
<segment>
<pinref part="LED1" gate="G$1" pin="+"/>
<pinref part="AR1" gate="G$1" pin="P1"/>
<wire x1="304.8" y1="-152.4" x2="304.8" y2="-154.94" width="0.1524" layer="91"/>
</segment>
</net>
<net name="N$23" class="0">
<segment>
<pinref part="AR1" gate="G$1" pin="P5"/>
<wire x1="314.96" y1="-152.4" x2="314.96" y2="-154.94" width="0.1524" layer="91"/>
<pinref part="LED3" gate="G$1" pin="+"/>
</segment>
</net>
<net name="N$20" class="0">
<segment>
<pinref part="U3" gate="G$1" pin="DPLUS"/>
<wire x1="208.28" y1="-53.34" x2="185.42" y2="-53.34" width="0.1524" layer="91"/>
<pinref part="J3" gate="G$1" pin="D+"/>
<wire x1="185.42" y1="-137.16" x2="172.894" y2="-137.16" width="0.1524" layer="91"/>
<wire x1="185.42" y1="-53.34" x2="185.42" y2="-137.16" width="0.1524" layer="91"/>
</segment>
</net>
<net name="N$28" class="0">
<segment>
<pinref part="U4" gate="G$1" pin="SDA"/>
<pinref part="U3" gate="G$1" pin="SDA"/>
<wire x1="165.1" y1="2.54" x2="180.34" y2="2.54" width="0.1524" layer="91"/>
<pinref part="R7" gate="G$1" pin="P$1"/>
<wire x1="180.34" y1="2.54" x2="208.28" y2="2.54" width="0.1524" layer="91"/>
<wire x1="180.34" y1="33.02" x2="180.34" y2="2.54" width="0.1524" layer="91"/>
<junction x="180.34" y="2.54"/>
</segment>
</net>
<net name="N$29" class="0">
<segment>
<pinref part="U4" gate="G$1" pin="SCL"/>
<pinref part="U3" gate="G$1" pin="SCL"/>
<wire x1="165.1" y1="7.62" x2="175.26" y2="7.62" width="0.1524" layer="91"/>
<pinref part="R5" gate="G$1" pin="P$1"/>
<wire x1="175.26" y1="7.62" x2="208.28" y2="7.62" width="0.1524" layer="91"/>
<wire x1="175.26" y1="33.02" x2="175.26" y2="7.62" width="0.1524" layer="91"/>
<junction x="175.26" y="7.62"/>
</segment>
</net>
<net name="RESET" class="0">
<segment>
<pinref part="U3" gate="G$1" pin="RESET#"/>
<pinref part="C21" gate="G$1" pin="1"/>
<wire x1="187.96" y1="-17.78" x2="208.28" y2="-17.78" width="0.1524" layer="91"/>
<wire x1="187.96" y1="-22.86" x2="187.96" y2="-17.78" width="0.1524" layer="91"/>
<label x="177.8" y="-15.24" size="1.778" layer="95"/>
<wire x1="187.96" y1="33.02" x2="187.96" y2="-17.78" width="0.1524" layer="91"/>
<junction x="187.96" y="-17.78"/>
<pinref part="R9" gate="G$1" pin="P$1"/>
</segment>
</net>
<net name="N$33" class="0">
<segment>
<pinref part="U3" gate="G$1" pin="DMINUS"/>
<wire x1="208.28" y1="-58.42" x2="190.5" y2="-58.42" width="0.1524" layer="91"/>
<pinref part="J3" gate="G$1" pin="D-"/>
<wire x1="190.5" y1="-142.24" x2="175.18" y2="-142.24" width="0.1524" layer="91"/>
<wire x1="175.18" y1="-142.24" x2="172.894" y2="-142.2" width="0.1524" layer="91"/>
<wire x1="190.5" y1="-58.42" x2="190.5" y2="-142.24" width="0.1524" layer="91"/>
</segment>
</net>
<net name="N$30" class="0">
<segment>
<pinref part="U3" gate="G$1" pin="WAKEUP"/>
<wire x1="198.12" y1="33.02" x2="198.12" y2="-22.86" width="0.1524" layer="91"/>
<wire x1="198.12" y1="-22.86" x2="208.28" y2="-22.86" width="0.1524" layer="91"/>
<pinref part="R8" gate="G$1" pin="P$1"/>
</segment>
</net>
<net name="N$34" class="0">
<segment>
<pinref part="J1" gate="G$1" pin="P2"/>
<wire x1="140.716" y1="33.02" x2="134.62" y2="33.02" width="0.1524" layer="91"/>
<pinref part="U4" gate="G$1" pin="A0"/>
<wire x1="139.7" y1="17.78" x2="134.62" y2="17.78" width="0.1524" layer="91"/>
<wire x1="134.62" y1="33.02" x2="134.62" y2="17.78" width="0.1524" layer="91"/>
</segment>
</net>
<net name="LED2" class="0">
<segment>
<pinref part="U3" gate="G$1" pin="PD7/FD15"/>
<pinref part="AR1" gate="G$1" pin="P6"/>
<wire x1="284.48" y1="-88.9" x2="314.96" y2="-88.9" width="0.1524" layer="91"/>
<wire x1="314.96" y1="-88.9" x2="314.96" y2="-147.32" width="0.1524" layer="91"/>
<label x="318.77" y="-109.474" size="1.778" layer="95" rot="R90"/>
</segment>
</net>
<net name="LED1" class="0">
<segment>
<pinref part="U3" gate="G$1" pin="PD6/FD14"/>
<pinref part="AR1" gate="G$1" pin="P4"/>
<wire x1="284.48" y1="-83.82" x2="309.88" y2="-83.82" width="0.1524" layer="91"/>
<wire x1="309.88" y1="-83.82" x2="309.88" y2="-147.32" width="0.1524" layer="91"/>
<label x="307.34" y="-109.22" size="1.778" layer="95" rot="R90"/>
</segment>
</net>
<net name="N$8" class="0">
<segment>
<pinref part="AR1" gate="G$1" pin="P3"/>
<pinref part="LED2" gate="G$1" pin="+"/>
<wire x1="309.88" y1="-152.4" x2="309.88" y2="-154.94" width="0.1524" layer="91"/>
</segment>
</net>
<net name="N$11" class="0">
<segment>
<pinref part="AR1" gate="G$1" pin="P7"/>
<wire x1="320.04" y1="-152.4" x2="320.04" y2="-154.94" width="0.1524" layer="91"/>
</segment>
</net>
<net name="FIFOADR1" class="0">
<segment>
<pinref part="U3" gate="G$1" pin="PA5/FIFOADR1"/>
<wire x1="284.48" y1="2.54" x2="289.56" y2="2.54" width="0.1524" layer="91"/>
<pinref part="R10" gate="G$1" pin="P$1"/>
<wire x1="289.56" y1="33.02" x2="289.56" y2="2.54" width="0.1524" layer="91"/>
</segment>
</net>
<net name="N$6" class="0">
<segment>
<pinref part="U3" gate="G$1" pin="RDY0/SLRD"/>
<wire x1="208.28" y1="-33.02" x2="203.2" y2="-33.02" width="0.1524" layer="91"/>
<wire x1="203.2" y1="-33.02" x2="203.2" y2="33.02" width="0.1524" layer="91"/>
<pinref part="R6" gate="G$1" pin="P$1"/>
</segment>
</net>
<net name="N$3" class="0">
<segment>
<pinref part="J4" gate="G$1" pin="RF"/>
<pinref part="BIAS_T" gate="G$1" pin="RF&amp;DC"/>
<wire x1="-196.85" y1="-50.8" x2="-193.04" y2="-50.8" width="0.1524" layer="91"/>
</segment>
</net>
</nets>
</sheet>
</sheets>
</schematic>
</drawing>
<compatibility>
<note version="6.3" minversion="6.2.2" severity="warning">
Since Version 6.2.2 text objects can contain more than one line,
which will not be processed correctly with this version.
</note>
</compatibility>
</eagle>
