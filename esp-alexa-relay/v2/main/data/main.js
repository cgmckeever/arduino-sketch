$( document ).ready(function() {

  var validationSettings = {
    rules: {
      device_name: {
        required: true
      },
      inching_delay: {
        required: true
      },
      led: {
        required: true
      }
    }
  };

  $.ajax({
         url: '/settings',
         success: function(data) {
           $.each(data, function(key, value, data) {
             var selector = '[name="' + key + '"]';
             var target = $(selector);
             if (target) {
              console.log(selector);
               if ( target.is("input") ) {
                target.val(value);
               } else {
                if (key == "triggered") {
                  if (value) {
                    value = "On";
                    var button = $('button[value="on"]')
                  } else {
                    value = "Off"
                    var button = $('button[value="off"]')
                  }
                  button.css("color", "grey");
                }
                target.html(value);
               }
             }
           });
         }
  });

  $.fn.serializeObject = function() {
    var o = {};
    var a = this.serializeArray();
    $.each(a, function() {
      var input = $("[name='" + this.name + "']");
      var value = this.value;
      var dataType = input[0].getAttribute("data-type");
      if (dataType == "number") value = parseFloat(value);
        
      if (o[this.name]) {
        if (!o[this.name].push) {
          o[this.name] = [o[this.name]];
        }
        o[this.name].push(value || '');
      } else {
        o[this.name] = value || '';
      }
    });
    return o;
  };

  var settingForm = '[name="settingsForm"]';
  $(settingForm).validate(validationSettings);
  $(settingForm).on('submit', function(e) {
    var target = $("[name='status']");
    target.innerHTML = "";
    if($(this).valid()) {
      e.preventDefault();

      var formData = $(this).serializeObject();

      // Send data as a PUT request
      $.ajax({
             url: '/settings',
             type: 'PUT',
             data: JSON.stringify(formData),
             contentType: 'application/json',
             success: function(data) {
               console.log(formData);
               target.getElementById("status").innerHTML = "Settings Updated"
             }
      });

      return false;
    }
  });
});  