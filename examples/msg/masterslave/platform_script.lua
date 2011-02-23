require "simgrid"

  simgrid.AS.new{id="AS0",mode="Full"}; 

  simgrid.AS.addHost{AS="AS0",id="Tremblay",power=98095000};
  simgrid.AS.addHost{AS="AS0",id="Jupiter",power=76296000};
  simgrid.AS.addHost{AS="AS0",id="Fafard",power=76296000};
  simgrid.AS.addHost{AS="AS0",id="Ginette",power=48492000};
  simgrid.AS.addHost{AS="AS0",id="Bourassa",power=48492000};

    -- create Links
  for i=10,0,-1 do
    simgrid.AS.addLink{AS="AS0",id=i,bandwidth=252750+ i*768,latency=0.000270544+i*0.087};   
  end
  -- simgrid.Route.new(src_id,des_id,links_nb,links_list)
   simgrid.AS.addRoute("AS0","Tremblay","Jupiter",{"1"});
   simgrid.AS.addRoute("AS0","Tremblay","Fafard",{"0","1","2","3","4","8"});
   simgrid.AS.addRoute("AS0","Tremblay","Ginette",{"3","4","5"});
   simgrid.AS.addRoute("AS0","Tremblay","Bourassa",{"0","1","3","2","4","6","7"});

   simgrid.AS.addRoute("AS0","Jupiter","Tremblay",{"1"});
   simgrid.AS.addRoute("AS0","Jupiter","Fafard",{"0","1","2","3","4","8","9"});
   simgrid.AS.addRoute("AS0","Jupiter","Ginette",{"3","4","5","9"});
   simgrid.AS.addRoute("AS0","Jupiter","Bourassa",{"0","1","2","3","4","6","7","9"});
 
   simgrid.AS.addRoute("AS0","Fafard","Tremblay",{"0","1","2","3","4","8"});
   simgrid.AS.addRoute("AS0","Fafard","Jupiter",{"0","1","2","3","4","8","9"});
   simgrid.AS.addRoute("AS0","Fafard","Ginette",{"0","1","2","5","8"});
   simgrid.AS.addRoute("AS0","Fafard","Bourassa",{"6","7","8"});
  
   simgrid.AS.addRoute("AS0","Ginette","Tremblay",{"3","4","5"});
   simgrid.AS.addRoute("AS0","Ginette","Jupiter",{"3","4","5","9"});
   simgrid.AS.addRoute("AS0","Ginette","Fafard",{"0","1","2","5","8"});
   simgrid.AS.addRoute("AS0","Ginette","Bourassa",{"0","1","2","5","6","7"});

   simgrid.AS.addRoute("AS0","Bourassa","Tremblay",{"0","1","3","2","4","6","7"});
   simgrid.AS.addRoute("AS0","Bourassa","Jupiter",{"0","1","2","3","4","6","7","9"});
   simgrid.AS.addRoute("AS0","Bourassa","Fafard",{"6","7","8"});
   simgrid.AS.addRoute("AS0","Bourassa","Ginette",{"0","1","2","5","6","7"});
  
   --Save Platform
   simgrid.msg_register_platform();

  --Set Application
   simgrid.Host.setFunction{host="Tremblay",fct="master",args="20,550000000,1000000,4"};
   simgrid.Host.setFunction{host="Bourassa",fct="slave",args="0"};
   simgrid.Host.setFunction{host="Jupiter",fct="slave",args="1"};
   simgrid.Host.setFunction{host="Fafard",fct="slave",args="2"};
   simgrid.Host.setFunction{host="Ginette",fct="slave",args="3"};
   
  --Save Application 
   simgrid.msg_register_application(); 


