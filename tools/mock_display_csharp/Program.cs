using System;
using System.Collections.Generic;
using System.Drawing;
using System.Linq;
using System.Windows.Forms;

namespace MockDisplay
{
    internal static class Program
    {
        [STAThread]
        private static void Main()
        {
            ApplicationConfiguration.Initialize();
            Application.Run(new MockDisplayForm());
        }
    }

    internal sealed class MockDisplayForm : Form
    {
        private const int ScreenW = 480;
        private const int ScreenH = 272;
        private const int HeaderH = 22;
        private const int RowH = 18;
        private const int KeyboardY = 150;
        private const int KeyW = 40;
        private const int KeyH = 28;

        private enum Screen
        {
            LiveMonitor = 0,
            ServiceMenu,
            ServiceIO,
            PinList,
            PinEdit,
            ReservedList,
            PwmList,
            Log,
            ConfirmCritical,
            RuleEdit,
            TouchTest
        }

        private enum EditType
        {
            Analog = 0,
            Digital,
            Output,
            Reserved,
            Pwm
        }

        private Screen _screen = Screen.LiveMonitor;
        private bool _dirty = true;
        private int _ioPage;
        private int _pinPage;
        private int _rulePage;
        private int _editingIndex;
        private int _editValue;
        private EditType _editType = EditType.Digital;
        private string _status = "";
        private bool _statusError;

        private readonly Queue<string> _log = new();

        private readonly List<int> _reservedPins = new();
        private readonly List<string> _reservedLabels = new();
        private readonly List<int> _pwmPins = new();
        private readonly List<string> _pwmLabels = new();

        private bool _kbVisible;
        private string _kbBuffer = "";

        private int _touchX;
        private int _touchY;
        private bool _touchActive;

        private readonly string[] _digitalLabels =
        {
            "Ignition ON","Ignition ACC","Ignition START","Brake Pedal","Handbrake",
            "Gear Park","Gear Reverse","Gear Neutral","Gear Drive",
            "Ind Left","Ind Right","Lights Parking","Lights Head","High Beam",
            "Hazard","Wiper Low","Wiper High","Wiper Int","Washer","Horn",
            "Demister","Trip Reset","Heat Elem 1 SW","Heat Elem 2 SW","Fan Low",
            "Fan Mid","Fan High","AC Off","AC Vent","AC Heat 1","AC Heat 2",
            "Wiper Start","Wiper Stop","Sprinkler","Lights Normal","Lights High","Lights Off"
        };

        private readonly int[] _digitalPins =
        {
            22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,48,49,50,51,52,53,55,56,57,58,59,60,61,62,63,64
        };

        private readonly string[] _outputLabels =
        {
            "Main Contactor","Brake Lights","Headlights","High Beams","Parking Lights",
            "Ind Left","Ind Right","Horn","Reverse Light","P-Brake Ind","Interior Light",
            "Charging Relay","Wiper Int","Wiper Low","Wiper High","Washer Pump",
            "Heater Elem 1","Heater Elem 2","Heater Fan","Demister"
        };

        private readonly int[] _outputPins =
        {
            2,4,6,7,8,14,15,16,17,18,19,43,44,45,46,47,0,1,3,5
        };

        public MockDisplayForm()
        {
            ClientSize = new Size(ScreenW, ScreenH);
            FormBorderStyle = FormBorderStyle.FixedSingle;
            MaximizeBox = false;
            Text = "Ellert Mock Display";
            DoubleBuffered = true;
            BackColor = Color.Black;
            MouseDown += OnMouseDown;
            Paint += OnPaint;
        }

        private void OnMouseDown(object? sender, MouseEventArgs e)
        {
            _touchX = e.X;
            _touchY = e.Y;
            _touchActive = true;

            if (_kbVisible && e.Y >= KeyboardY)
            {
                HandleKeyboardTouch(e.X, e.Y);
                return;
            }

            HandleTouch(e.X, e.Y);
        }

        private void HandleKeyboardTouch(int x, int y)
        {
            int row = (y - KeyboardY) / KeyH;
            int col = x / KeyW;

            string row1 = "1234567890";
            string row2 = "QWERTYUIOP";
            string row3 = "ASDFGHJKL";
            string row4 = "ZXCVBNM";

            if (row == 0 && col < 10) AppendKey(row1[col]);
            else if (row == 1 && col < 10) AppendKey(row2[col]);
            else if (row == 2 && col < 9) AppendKey(row3[col]);
            else if (row == 3)
            {
                if (col < 7) AppendKey(row4[col]);
                else if (col == 7) AppendKey('_');
                else if (col == 8) BackspaceKey();
                else if (col == 9) CommitKeyboard();
            }

            Invalidate();
        }

        private void AppendKey(char c)
        {
            if (_kbBuffer.Length < 12) _kbBuffer += c;
        }

        private void BackspaceKey()
        {
            if (_kbBuffer.Length > 0) _kbBuffer = _kbBuffer[..^1];
        }

        private void CommitKeyboard()
        {
            if (_editType == EditType.Reserved)
            {
                if (_editingIndex < _reservedLabels.Count) _reservedLabels[_editingIndex] = _kbBuffer;
            }
            else if (_editType == EditType.Pwm)
            {
                if (_editingIndex < _pwmLabels.Count) _pwmLabels[_editingIndex] = _kbBuffer;
            }
            Log("Label saved", false);
            _kbVisible = false;
        }

        private void HandleTouch(int x, int y)
        {
            if (_screen == Screen.LiveMonitor)
            {
                if (Hit(400, 0, 80, HeaderH, x, y)) { _screen = Screen.ServiceMenu; _dirty = true; }
                return;
            }

            if (_screen == Screen.ServiceMenu)
            {
                if (Hit(0, 0, 60, HeaderH, x, y)) { _screen = Screen.LiveMonitor; _dirty = true; return; }
                if (Hit(40, 50, 200, RowH, x, y)) { _screen = Screen.ServiceIO; _dirty = true; return; }
                if (Hit(40, 80, 200, RowH, x, y)) { _screen = Screen.PinList; _dirty = true; return; }
                if (Hit(40, 110, 200, RowH, x, y)) { _screen = Screen.ReservedList; _dirty = true; return; }
                if (Hit(40, 140, 200, RowH, x, y)) { _screen = Screen.PwmList; _dirty = true; return; }
                if (Hit(40, 170, 200, RowH, x, y)) { _screen = Screen.Log; _dirty = true; return; }
                if (Hit(40, 200, 200, RowH, x, y)) { _screen = Screen.TouchTest; _dirty = true; return; }
            }

            if (_screen == Screen.ServiceIO)
            {
                if (Hit(0, 0, 60, HeaderH, x, y)) { _screen = Screen.ServiceMenu; _dirty = true; return; }
                if (Hit(380, 230, 80, 30, x, y)) { _ioPage++; _dirty = true; return; }
                if (Hit(300, 230, 80, 30, x, y)) { _ioPage = Math.Max(0, _ioPage - 1); _dirty = true; return; }
            }

            if (_screen == Screen.PinList)
            {
                if (Hit(0, 0, 60, HeaderH, x, y)) { _screen = Screen.ServiceMenu; _dirty = true; return; }
                if (Hit(180, 230, 110, 30, x, y)) { Log("Defaults restored", false); _dirty = true; return; }
                if (Hit(380, 230, 80, 30, x, y)) { _pinPage++; _dirty = true; return; }
                if (Hit(300, 230, 80, 30, x, y)) { _pinPage = Math.Max(0, _pinPage - 1); _dirty = true; return; }

                int startY = HeaderH + 10;
                int perPage = 9;
                int total = 1 + _digitalLabels.Length + _outputLabels.Length;
                int start = _pinPage * perPage;
                for (int i = 0; i < perPage; i++)
                {
                    int idx = start + i;
                    if (idx >= total) break;
                    int rowY = startY + i * RowH;
                    if (!Hit(10, rowY, 460, RowH, x, y)) continue;

                    if (idx == 0) { _editType = EditType.Analog; _editingIndex = 0; _editValue = 54; }
                    else if (idx <= _digitalLabels.Length) { _editType = EditType.Digital; _editingIndex = idx - 1; _editValue = _digitalPins[_editingIndex]; }
                    else { _editType = EditType.Output; _editingIndex = idx - 1 - _digitalLabels.Length; _editValue = _outputPins[_editingIndex]; }

                    _screen = Screen.PinEdit; _dirty = true; return;
                }
            }

            if (_screen == Screen.PinEdit)
            {
                if (Hit(0, 0, 60, HeaderH, x, y)) { _screen = Screen.PinList; _dirty = true; return; }
                if (Hit(80, 80, 60, 30, x, y)) { _editValue--; _dirty = true; return; }
                if (Hit(200, 80, 60, 30, x, y)) { _editValue++; _dirty = true; return; }
                if (Hit(180, 210, 120, 30, x, y)) { _screen = Screen.PinList; _dirty = true; return; }
                if (Hit(320, 210, 120, 30, x, y))
                {
                    if (_editType == EditType.Output && (_editingIndex == 0 || _editingIndex == 1))
                    {
                        _screen = Screen.ConfirmCritical; _dirty = true; return;
                    }
                    Log("Saved pin mapping", false);
                    _screen = Screen.PinList; _dirty = true; return;
                }
            }

            if (_screen == Screen.ConfirmCritical)
            {
                if (Hit(100, 200, 100, 30, x, y)) { _screen = Screen.PinEdit; _dirty = true; return; }
                if (Hit(260, 200, 120, 30, x, y)) { Log("Saved critical output", false); _screen = Screen.PinList; _dirty = true; return; }
            }

            if (_screen == Screen.ReservedList || _screen == Screen.PwmList)
            {
                if (Hit(0, 0, 60, HeaderH, x, y)) { _screen = Screen.ServiceMenu; _dirty = true; return; }
                if (Hit(10, 230, 60, 30, x, y))
                {
                    if (_screen == Screen.ReservedList) { _reservedPins.Clear(); _reservedLabels.Clear(); Log("Reserved cleared", false); }
                    else { _pwmPins.Clear(); _pwmLabels.Clear(); Log("PWM cleared", false); }
                    _dirty = true; return;
                }
                if (Hit(80, 230, 90, 30, x, y))
                {
                    if (_screen == Screen.ReservedList && _reservedPins.Count > 0) { _reservedPins.RemoveAt(_reservedPins.Count - 1); _reservedLabels.RemoveAt(_reservedLabels.Count - 1); }
                    if (_screen == Screen.PwmList && _pwmPins.Count > 0) { _pwmPins.RemoveAt(_pwmPins.Count - 1); _pwmLabels.RemoveAt(_pwmLabels.Count - 1); }
                    _dirty = true; return;
                }
                if (Hit(180, 230, 110, 30, x, y))
                {
                    if (_screen == Screen.ReservedList) { _reservedPins.Add(0); _reservedLabels.Add(""); }
                    else { _pwmPins.Add(0); _pwmLabels.Add(""); }
                    _dirty = true; return;
                }
                if (Hit(300, 230, 80, 30, x, y)) { _rulePage = Math.Max(0, _rulePage - 1); _dirty = true; return; }
                if (Hit(380, 230, 80, 30, x, y)) { _rulePage++; _dirty = true; return; }

                int startY = HeaderH + 10;
                int perPage = 9;
                int total = _screen == Screen.ReservedList ? _reservedPins.Count : _pwmPins.Count;
                int start = _rulePage * perPage;
                for (int i = 0; i < perPage; i++)
                {
                    int idx = start + i;
                    if (idx >= total) break;
                    int rowY = startY + i * RowH;
                    if (!Hit(10, rowY, 460, RowH, x, y)) continue;
                    _editType = _screen == Screen.ReservedList ? EditType.Reserved : EditType.Pwm;
                    _editingIndex = idx;
                    _editValue = _screen == Screen.ReservedList ? _reservedPins[idx] : _pwmPins[idx];
                    _screen = Screen.RuleEdit; _dirty = true; return;
                }
            }

            if (_screen == Screen.RuleEdit)
            {
                if (Hit(0, 0, 60, HeaderH, x, y)) { _screen = _editType == EditType.Reserved ? Screen.ReservedList : Screen.PwmList; _dirty = true; return; }
                if (Hit(80, 80, 60, 30, x, y)) { _editValue--; _dirty = true; return; }
                if (Hit(200, 80, 60, 30, x, y)) { _editValue++; _dirty = true; return; }
                if (Hit(100, 140, 80, 30, x, y)) { CycleLabel(); _dirty = true; return; }
                if (Hit(200, 140, 80, 30, x, y)) { _kbVisible = true; _kbBuffer = GetLabel(); _dirty = true; return; }
                if (Hit(180, 210, 120, 30, x, y)) { _screen = _editType == EditType.Reserved ? Screen.ReservedList : Screen.PwmList; _dirty = true; return; }
                if (Hit(320, 210, 120, 30, x, y))
                {
                    if (_editType == EditType.Reserved) _reservedPins[_editingIndex] = _editValue;
                    else _pwmPins[_editingIndex] = _editValue;
                    Log("Rule saved", false);
                    _screen = _editType == EditType.Reserved ? Screen.ReservedList : Screen.PwmList;
                    _dirty = true; return;
                }
            }

            if (_screen == Screen.Log)
            {
                if (Hit(0, 0, 60, HeaderH, x, y)) { _screen = Screen.ServiceMenu; _dirty = true; return; }
            }

            if (_screen == Screen.TouchTest)
            {
                if (Hit(0, 0, 60, HeaderH, x, y)) { _screen = Screen.ServiceMenu; _dirty = true; return; }
            }
        }

        private void CycleLabel()
        {
            string[] presets = _editType == EditType.Reserved
                ? new[] { "SPI", "I2C", "TOUCH", "SER", "LCD", "AUX", "RESV", "" }
                : new[] { "PWM1", "PWM2", "PWM3", "FAN", "LED", "AUX", "" };

            string current = GetLabel();
            int idx = Array.IndexOf(presets, current);
            int next = (idx + 1) % presets.Length;
            SetLabel(presets[next]);
            Log("Label set", false);
        }

        private string GetLabel()
        {
            return _editType == EditType.Reserved ? _reservedLabels[_editingIndex] : _pwmLabels[_editingIndex];
        }

        private void SetLabel(string value)
        {
            if (_editType == EditType.Reserved) _reservedLabels[_editingIndex] = value;
            else _pwmLabels[_editingIndex] = value;
        }

        private void Log(string msg, bool isError)
        {
            _status = msg;
            _statusError = isError;
            if (_log.Count >= 10) _log.Dequeue();
            _log.Enqueue(msg);
            _dirty = true;
        }

        private bool Hit(int x, int y, int w, int h, int tx, int ty)
        {
            return tx >= x && tx <= x + w && ty >= y && ty <= y + h;
        }

        private void OnPaint(object? sender, PaintEventArgs e)
        {
            if (_dirty) { }
            DrawScreen(e.Graphics);
        }

        private void DrawScreen(Graphics g)
        {
            g.Clear(Color.Black);
            switch (_screen)
            {
                case Screen.LiveMonitor: DrawLive(g); break;
                case Screen.ServiceMenu: DrawServiceMenu(g); break;
                case Screen.ServiceIO: DrawServiceIO(g); break;
                case Screen.PinList: DrawPinList(g); break;
                case Screen.PinEdit: DrawPinEdit(g); break;
                case Screen.ReservedList: DrawRuleList(g, true); break;
                case Screen.PwmList: DrawRuleList(g, false); break;
                case Screen.Log: DrawLog(g); break;
                case Screen.ConfirmCritical: DrawConfirm(g); break;
                case Screen.RuleEdit: DrawRuleEdit(g); break;
                case Screen.TouchTest: DrawTouchTest(g); break;
            }
            if (_kbVisible) DrawKeyboard(g);
        }

        private void DrawHeader(Graphics g, string title)
        {
            g.FillRectangle(Brushes.Black, 0, 0, ScreenW, HeaderH);
            g.DrawString(title, Font, Brushes.White, 5, 5);
        }

        private void DrawButton(Graphics g, int x, int y, int w, int h, string label, bool filled)
        {
            if (filled) g.FillRectangle(Brushes.Blue, x, y, w, h);
            else g.DrawRectangle(Pens.White, x, y, w, h);
            g.DrawString(label, Font, Brushes.White, x + 4, y + 5);
        }

        private void DrawStatus(Graphics g, int x, int y, string label, int pin, bool isInput)
        {
            g.DrawString($"{label} (D{pin}):", Font, Brushes.White, x, y);
            g.DrawString(isInput ? "LOW" : "HIGH", Font, Brushes.Lime, x + 200, y);
        }

        private void DrawLive(Graphics g)
        {
            DrawHeader(g, "LIVE I/O MONITOR");
            DrawButton(g, 400, 0, 80, HeaderH, "SERVICE", true);
            int row = HeaderH + 8;
            DrawStatus(g, 5, row, "Ignition ON", 22, true); row += RowH;
            DrawStatus(g, 5, row, "Brake Pedal", 25, true); row += RowH;
            DrawStatus(g, 5, row, "Handbrake", 26, true); row += RowH;
            DrawStatus(g, 5, row, "Gear D", 30, true); row += RowH + 4;
            DrawStatus(g, 5, row, "Main Contactor", 2, false); row += RowH;
            DrawStatus(g, 5, row, "Brake Lights", 4, false); row += RowH + 4;
            DrawStatus(g, 5, row, "Ind Left", 14, false); row += RowH;
            DrawStatus(g, 5, row, "Ind Right", 15, false); row += RowH;
        }

        private void DrawServiceMenu(Graphics g)
        {
            DrawHeader(g, "SERVICE MENU");
            DrawButton(g, 0, 0, 60, HeaderH, "BACK", false);
            DrawButton(g, 40, 50, 200, RowH, "I/O Monitor", true);
            DrawButton(g, 40, 80, 200, RowH, "Pin Remap", true);
            DrawButton(g, 40, 110, 200, RowH, "Reserved Pins", true);
            DrawButton(g, 40, 140, 200, RowH, "PWM Pins", true);
            DrawButton(g, 40, 170, 200, RowH, "Log", true);
            DrawButton(g, 40, 200, 200, RowH, "Touch Test", true);
        }

        private void DrawServiceIO(Graphics g)
        {
            DrawHeader(g, "SERVICE I/O");
            DrawButton(g, 0, 0, 60, HeaderH, "BACK", false);
            DrawButton(g, 300, 230, 80, 30, "PREV", false);
            DrawButton(g, 380, 230, 80, 30, "NEXT", false);
        }

        private void DrawPinList(Graphics g)
        {
            DrawHeader(g, "PIN REMAP");
            DrawButton(g, 0, 0, 60, HeaderH, "BACK", false);
            DrawButton(g, 180, 230, 110, 30, "DEFAULTS", false);
            DrawButton(g, 300, 230, 80, 30, "PREV", false);
            DrawButton(g, 380, 230, 80, 30, "NEXT", false);

            int row = HeaderH + 10;
            int perPage = 9;
            int total = 1 + _digitalLabels.Length + _outputLabels.Length;
            int start = _pinPage * perPage;
            for (int i = 0; i < perPage; i++)
            {
                int idx = start + i;
                if (idx >= total) break;
                if (idx == 0)
                {
                    g.DrawString("[ANA] Accelerator = A0 (D54)", Font, Brushes.White, 12, row);
                }
                else if (idx <= _digitalLabels.Length)
                {
                    g.DrawString($"[IN ] {_digitalLabels[idx - 1]} = D{_digitalPins[idx - 1]}", Font, Brushes.White, 12, row);
                }
                else
                {
                    int o = idx - 1 - _digitalLabels.Length;
                    g.DrawString($"[OUT] {_outputLabels[o]} = D{_outputPins[o]}", Font, Brushes.White, 12, row);
                }
                row += RowH;
            }

            if (!string.IsNullOrEmpty(_status))
            {
                using var brush = _statusError ? Brushes.Red : Brushes.Lime;
                g.DrawString(_status, Font, brush, 12, 210);
            }
        }

        private void DrawPinEdit(Graphics g)
        {
            DrawHeader(g, "EDIT PIN");
            DrawButton(g, 0, 0, 60, HeaderH, "BACK", false);
            string title = _editType switch
            {
                EditType.Analog => "Analog: Accelerator",
                EditType.Digital => "Input: " + _digitalLabels[_editingIndex],
                _ => "Output: " + _outputLabels[_editingIndex]
            };
            g.DrawString(title, Font, Brushes.White, 10, 40);
            g.DrawString(_editType == EditType.Analog ? "Pin: A0 (D54)" : $"Pin: D{_editValue}", Font, Brushes.White, 10, 65);
            DrawButton(g, 80, 80, 60, 30, "-", true);
            DrawButton(g, 200, 80, 60, 30, "+", true);
            DrawButton(g, 180, 210, 120, 30, "CANCEL", false);
            DrawButton(g, 320, 210, 120, 30, "SAVE", true);
        }

        private void DrawRuleList(Graphics g, bool reserved)
        {
            DrawHeader(g, reserved ? "RESERVED PINS" : "PWM PINS");
            DrawButton(g, 0, 0, 60, HeaderH, "BACK", false);
            DrawButton(g, 10, 230, 60, 30, "CLR", false);
            DrawButton(g, 80, 230, 90, 30, "DEL", false);
            DrawButton(g, 180, 230, 110, 30, "ADD", false);
            DrawButton(g, 300, 230, 80, 30, "PREV", false);
            DrawButton(g, 380, 230, 80, 30, "NEXT", false);

            int row = HeaderH + 10;
            int perPage = 9;
            int total = reserved ? _reservedPins.Count : _pwmPins.Count;
            int start = _rulePage * perPage;
            for (int i = 0; i < perPage; i++)
            {
                int idx = start + i;
                if (idx >= total) break;
                int pin = reserved ? _reservedPins[idx] : _pwmPins[idx];
                string label = reserved ? _reservedLabels[idx] : _pwmLabels[idx];
                g.DrawString($"D{pin} {label}", Font, Brushes.White, 12, row);
                row += RowH;
            }
        }

        private void DrawRuleEdit(Graphics g)
        {
            DrawHeader(g, "EDIT RULE");
            DrawButton(g, 0, 0, 60, HeaderH, "BACK", false);
            g.DrawString(_editType == EditType.Reserved ? "Reserved pin" : "PWM pin", Font, Brushes.White, 10, 40);
            g.DrawString($"Pin: D{_editValue}", Font, Brushes.White, 10, 65);
            g.DrawString($"Label: {GetLabel()}", Font, Brushes.White, 10, 110);
            g.DrawString("Tap LABEL or TYPE", Font, Brushes.White, 10, 125);
            DrawButton(g, 80, 80, 60, 30, "-", true);
            DrawButton(g, 200, 80, 60, 30, "+", true);
            DrawButton(g, 100, 140, 80, 30, "LABEL", false);
            DrawButton(g, 200, 140, 80, 30, "TYPE", false);
            DrawButton(g, 180, 210, 120, 30, "CANCEL", false);
            DrawButton(g, 320, 210, 120, 30, "SAVE", true);
        }

        private void DrawLog(Graphics g)
        {
            DrawHeader(g, "SERVICE LOG");
            DrawButton(g, 0, 0, 60, HeaderH, "BACK", false);
            int row = HeaderH + 10;
            foreach (string msg in _log.TakeLast(9))
            {
                g.DrawString(msg, Font, Brushes.White, 12, row);
                row += RowH;
            }
        }

        private void DrawConfirm(Graphics g)
        {
            DrawHeader(g, "CONFIRM CHANGE");
            g.DrawString("Critical output change!", Font, Brushes.Red, 10, 60);
            g.DrawString("Are you sure?", Font, Brushes.White, 10, 80);
            DrawButton(g, 100, 200, 100, 30, "CANCEL", false);
            DrawButton(g, 260, 200, 120, 30, "CONFIRM", true);
        }

        private void DrawTouchTest(Graphics g)
        {
            DrawHeader(g, "TOUCH TEST");
            DrawButton(g, 0, 0, 60, HeaderH, "BACK", false);
            g.DrawString($"Touch: {(_touchActive ? "YES" : "NO")}", Font, Brushes.White, 10, 40);
            g.DrawString($"X: {_touchX}  Y: {_touchY}", Font, Brushes.White, 10, 60);
            if (_touchActive) g.FillEllipse(Brushes.Lime, _touchX - 3, _touchY - 3, 6, 6);
        }

        private void DrawKeyboard(Graphics g)
        {
            g.FillRectangle(Brushes.Black, 0, KeyboardY, ScreenW, ScreenH - KeyboardY);
            g.DrawRectangle(Pens.White, 0, KeyboardY, ScreenW, ScreenH - KeyboardY);
            string row1 = "1234567890";
            string row2 = "QWERTYUIOP";
            string row3 = "ASDFGHJKL";
            string row4 = "ZXCVBNM";

            for (int i = 0; i < 10; i++) g.DrawString(row1[i].ToString(), Font, Brushes.White, i * KeyW + 8, KeyboardY + 6);
            for (int i = 0; i < 10; i++) g.DrawString(row2[i].ToString(), Font, Brushes.White, i * KeyW + 8, KeyboardY + KeyH + 6);
            for (int i = 0; i < 9; i++) g.DrawString(row3[i].ToString(), Font, Brushes.White, i * KeyW + 8, KeyboardY + KeyH * 2 + 6);
            for (int i = 0; i < 7; i++) g.DrawString(row4[i].ToString(), Font, Brushes.White, i * KeyW + 8, KeyboardY + KeyH * 3 + 6);

            g.DrawString("_", Font, Brushes.White, 7 * KeyW + 6, KeyboardY + KeyH * 3 + 6);
            g.DrawString("<", Font, Brushes.White, 8 * KeyW + 6, KeyboardY + KeyH * 3 + 6);
            g.DrawString("OK", Font, Brushes.White, 9 * KeyW + 6, KeyboardY + KeyH * 3 + 6);

            g.DrawString($"Label: {_kbBuffer}", Font, Brushes.White, 10, KeyboardY - 12);
        }
    }
}
