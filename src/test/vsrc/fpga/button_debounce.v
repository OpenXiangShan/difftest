module button_debounce(
     input            clk,
	 input            rstn,
     input            button_i,
     output           button_rflag	 
);

//
parameter        IDLE         =    3'b000;
parameter        FILTER       =    3'b001;
parameter        CHECK_IN     =    3'b011;
parameter        STAYLOW      =    3'b010;
parameter        FILTER_O     =    3'b110;
parameter        CHECK_OUT    =    3'b100;
parameter        DONE         =    3'b101;


(*mark_debug = "true"*) reg         [2:0]       cur_state, nxt_state;
(*mark_debug = "true"*) reg                     button_d1, button_d2;
(*mark_debug = "true"*) wire                    button_f, button_r;
always@(posedge clk) begin
	if (!rstn) begin
	   button_d1 <= 1'b1;
	   button_d2 <= 1'b1;
	end
	else begin
           button_d1 <= button_i;
           button_d2 <= button_d1;
        end
end
assign button_f = ~button_d1 && button_d2;
assign button_r = button_d1 && ~button_d2;

(*mark_debug = "true"*) reg  [19:0]   counter;
always@(posedge clk) begin
   if ((cur_state==FILTER)||(cur_state==FILTER_O))
	   counter <= counter + 1'b1;
   else
	   counter <= 0;
end

always@(posedge clk) begin
   if (!rstn)
      cur_state <= IDLE;
   else
      cur_state <= nxt_state;
end

always@(*) begin
   case(cur_state)
      IDLE:
	  if(button_f)
	      nxt_state = FILTER;
          else
              nxt_state = IDLE;
      FILTER:
	  if(counter==20'h7a120)   // 10ms 50MHz
              nxt_state = CHECK_IN;
          else
	      nxt_state = FILTER;
      CHECK_IN:
	  if(button_d2)
	     nxt_state = FILTER;
          else
             nxt_state = STAYLOW;
      STAYLOW:
	  if(button_r)   
	     nxt_state = FILTER_O;
           else
             nxt_state = STAYLOW;
      FILTER_O:
	  if(counter==20'h7a120)
		  nxt_state = CHECK_OUT;
	  else
		  nxt_state = FILTER_O;
      CHECK_OUT:
	  if(button_d2)
	      nxt_state = DONE;
          else
	      nxt_state = FILTER_O;
      DONE:
	      nxt_state = IDLE;

   endcase
end
assign button_rflag = (cur_state == DONE);


endmodule
