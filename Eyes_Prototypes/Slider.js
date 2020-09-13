/*----------------------------------------------------------------------
         _          __________                              _,
     _.-(_)._     ."          ".      .--""--.          _.-{__}-._
   .'________'.   | .--------. |    .'        '.      .:-'`____`'-:.
  [____________] /` |________| `\  /   .'``'.   \    /_.-"`_  _`"-._\
  /  / .\/. \  \|  / / .\/. \ \  ||  .'/.\/.\'.  |  /`   / .\/. \   `\
  |  \__/\__/  |\_/  \__/\__/  \_/|  : |_/\_| ;  |  |    \__/\__/    |
  \            /  \            /   \ '.\    /.' / .-\                /-.
  /'._  --  _.'\  /'._  --  _.'\   /'. `'--'` .'\/   '._-.__--__.-_.'   \
 /_   `""""`   _\/_   `""""`   _\ /_  `-./\.-'  _\'.    `""""""""`    .'`\
(__/    '|    \ _)_|           |_)_/            \__)|        '       |   |
  |_____'|_____|   \__________/   |              |;`_________'________`;-'
   '----------'    '----------'   '--------------'`--------------------`
------------------------------------------------------------------------
 ___ _ _    _           _
/ __| (_)__| |___ _ _  (_)___
\__ \ | / _` / -_) '_| | (_-<
|___/_|_\__,_\___|_|(_)/ /__/
                     |__/
------------------------------------------------------------------------
  Slider Madness
  For Ambisynth project
  Made by Michael Krzyzaniak at University of Surrey's
  Center for Vision, Speech and Signal Processing
  in Fall of 2018
  m.krzyzaniak@surrey.ac.uk
  michael.krzyzaniak@yahoo.com

  the public interface is intended to be:
  var slider = new Slider(parent_id, min, max, initial_value, label, type, action);
  //type is "linear" or "exponential"
  //action is function to be called oninput with slidr value as last arg
  slider.get_step();
  slider.get_step(step);
  slider.get_label();
  slider.set_label(step);
  slider.set_value(val);
  slider.get_value(val);

----------------------------------------------------------------------*/

function Slider(parent_id, min, max, value, label, type, action)
{
  var div             = document.createElement("div");
  div.className       = "slider_container_div";
  div.style.textAlign = "center";

  var slider          = document.createElement("input");
  slider.className    = "slider_div unselected";
  slider.type         = "range";
  slider.min          = min;
  slider.max          = max;
  slider.step         = 0.001;
  slider.value        = min;
  slider.id           = label;
  slider.oninput      = this.oninput.bind(this);

  var label_div       = document.createElement("div");
  label_div.className = "slider_label_div";
  label_div.style     = "position:relative; top:-20px;";
  
  var label_span       = document.createElement("span");
  label_span.className = "slider_label_span";

  var number_span      = document.createElement("span");
  label_span.className = "slider_number_span";
  
  var parent_element   = document.getElementById(parent_id);
  
  this.container       = div;
  this.slider          = slider;
  this.label           = label_span;
  this.number          = number_span;
  this.type            = type;
  this.action          = action;
  
  this.set_label(label);
  this.set_value(value, false);
  
  div.appendChild(slider);
  div.appendChild(label_div);
  label_div.appendChild(label_span);
  label_div.appendChild(number_span);
  if(parent_element)
    parent_element.appendChild(div);
}

/*--------------------------------------------------------------------*/
Slider.prototype.get_step = function()
{
  return this.slider.step;
};

/*--------------------------------------------------------------------*/
Slider.prototype.set_step = function(step)
{
  this.slider.step = step;
};

/*--------------------------------------------------------------------*/
Slider.prototype.get_label = function()
{
  var label = this.label.innerHTML;
  return label.substring(0, label.length-2); //remove the ": "
};

/*--------------------------------------------------------------------*/
Slider.prototype.set_label = function(label)
{
  this.label.innerHTML = label + ": ";
};

/*--------------------------------------------------------------------*/
Slider.prototype.get_value = function()
{
  var x = parseFloat(this.slider.value);
  var result = 0;

  if(this.type=="exponential")
    {
      var a = parseFloat(this.slider.min);
      var b = parseFloat(this.slider.max);
      var result = Math.pow((x-a)/(a-b), 2);
      result *= b-a;
      result += a;
    }
  else //(type == "linear")
    result = x;
  
  return result;
};

/*--------------------------------------------------------------------*/
Slider.prototype.set_value = function(val, should_perform_action)
{
  var x = 0;
  
  if(val < this.slider.min) val = this.slider.min;
  if(val > this.slider.max) val = this.slider.max;

  if(this.type=="exponential")
    {
      var a = parseFloat(this.slider.min);
      var b = parseFloat(this.slider.max);
      var y = val;
      x = 0;
      if(y != 0)
        x = -Math.sqrt((y-a)/(b-a));
      x *= a-b;
      x += a;
    }
  else //(type == "linear")
    x = val;

  this.slider.value = x;
  
  if(should_perform_action)
    this.oninput();
  else
   var val = this.get_value();
   this.number.innerHTML = val.toFixed(3);
};

/*--------------------------------------------------------------------*/
Slider.prototype.oninput = function()
{
  var val = this.get_value();
  this.number.innerHTML = val.toFixed(3);
  //var target - this.target;
  this.action(val);
};

