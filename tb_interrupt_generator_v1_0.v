
`timescale 1 ns / 1 ps

module tb_interrupt_generator_v1_0();

localparam AXI_CLK_PERIOD = 20; // 20 ns -> 50 MHz

parameter integer C_S00_AXI_DATA_WIDTH	= 32;
parameter integer C_S00_AXI_ADDR_WIDTH	= 6;

reg                                 s00_axi_aclk;
reg                                 s00_axi_aresetn;
reg [C_S00_AXI_ADDR_WIDTH-1:0]      s00_axi_awaddr;
reg [2:0]                           s00_axi_awprot;
reg                                 s00_axi_awvalid;
wire                                s00_axi_awready;
reg [C_S00_AXI_DATA_WIDTH-1:0]      s00_axi_wdata;
reg [(C_S00_AXI_DATA_WIDTH/8)-1:0]  s00_axi_wstrb;
reg                                 s00_axi_wvalid;
wire                                s00_axi_wready;
wire [1:0]                          s00_axi_bresp;
wire                                s00_axi_bvalid;
reg                                 s00_axi_bready;
reg [C_S00_AXI_ADDR_WIDTH-1:0]      s00_axi_araddr;
reg [2:0]                           s00_axi_arprot;
reg                                 s00_axi_arvalid;
wire                                s00_axi_arready;
wire [C_S00_AXI_DATA_WIDTH-1:0]     s00_axi_rdata;
wire [1:0]                          s00_axi_rresp;
wire                                s00_axi_rvalid;
reg                                 s00_axi_rready;

wire                                int_out;

interrupt_generator_v1_0 DUT
( 
    .int_out(int_out),
    
    .s00_axi_aclk(s00_axi_aclk),
    .s00_axi_aresetn(s00_axi_aresetn),
    .s00_axi_awaddr(s00_axi_awaddr),
    .s00_axi_awprot(s00_axi_awprot),
    .s00_axi_awvalid(s00_axi_awvalid),
    .s00_axi_awready(s00_axi_awready),
    .s00_axi_wdata(s00_axi_wdata),
    .s00_axi_wstrb(s00_axi_wstrb),
    .s00_axi_wvalid(s00_axi_wvalid),
    .s00_axi_wready(s00_axi_wready),
    .s00_axi_bresp(s00_axi_bresp),
    .s00_axi_bvalid(s00_axi_bvalid),
    .s00_axi_bready(s00_axi_bready),
    .s00_axi_araddr(s00_axi_araddr),
    .s00_axi_arprot(s00_axi_arprot),
    .s00_axi_arvalid(s00_axi_arvalid),
    .s00_axi_arready(s00_axi_arready),
    .s00_axi_rdata(s00_axi_rdata),
    .s00_axi_rresp(s00_axi_rresp),
    .s00_axi_rvalid(s00_axi_rvalid),
    .s00_axi_rready(s00_axi_rready)
);

initial begin
    s00_axi_aclk = 1'b0;
    forever #(AXI_CLK_PERIOD / 2) s00_axi_aclk = ~s00_axi_aclk;
end

initial begin
    s00_axi_aresetn = 1'b0;
    repeat (5) @(posedge s00_axi_aclk);

    s00_axi_aresetn = 1'b1;
    @(posedge s00_axi_aclk);
end

initial begin
    s00_axi_awaddr = 0;

    s00_axi_bready = 0;

    s00_axi_wvalid = 0;
    s00_axi_awvalid = 0;
    s00_axi_awprot = 0;
    s00_axi_arvalid = 0;
    s00_axi_wdata = 0;
    s00_axi_wstrb = 4'hF;
    s00_axi_rready = 0;
    s00_axi_araddr = 0;
    s00_axi_arprot = 0;

    repeat (10) @(posedge s00_axi_aclk);

    // Read constant value
    axi_read(32'h0000_0000);
    
    // Interrupt Enable
    axi_write(32'h0000_0004, 32'h0000_0001);
    
    // Read Interrupt Enable Register
    axi_read(32'h0000_0004);
    
    // Generate Interrupt
    axi_write(32'h0000_0010, 32'h0000_0001);
    
    // Read Interrupt Status
    axi_read(32'h0000_000C);
    
    // Clear Interrupt
    axi_write(32'h0000_0008, 32'h0000_0001);
end

task axi_write;
    input [31:0] reg_addr;
    input [31:0] reg_data;
begin
    s00_axi_awprot = 3'b000;
    s00_axi_wstrb = 4'hF;
    s00_axi_bready = 1'b0;

    @(posedge s00_axi_aclk);
    s00_axi_awaddr = reg_addr;
    s00_axi_awvalid = 1'b1;

    s00_axi_wdata = reg_data;
    s00_axi_wvalid = 1'b1;
    s00_axi_bready = 1;

    while (!(s00_axi_awready && s00_axi_wready))
        @(posedge s00_axi_aclk);

    @(posedge s00_axi_aclk);
    s00_axi_awvalid = 1'b0;
    s00_axi_wvalid = 1'b0;

    while (!s00_axi_bvalid)
        @(posedge s00_axi_aclk);

    @(posedge s00_axi_aclk);
    s00_axi_bready = 1'b0;

    $display ("[%g] AXI_WRITE Addr: 0x%08h | Value: 0x%08h", $time, reg_addr, reg_data);
end
endtask

task axi_read;
    input [31:0] reg_addr;
    reg [31:0] reg_value;
begin

    s00_axi_arprot = 3'b000;
    s00_axi_rready = 1'b0;

    @(posedge s00_axi_aclk);
    s00_axi_araddr = reg_addr;
    s00_axi_arvalid = 1'b1;

    while (!s00_axi_arready)
        @(posedge s00_axi_aclk);

    @(posedge s00_axi_aclk);
    s00_axi_arvalid = 1'b0;

    s00_axi_rready = 1'b1;

    while (!s00_axi_rvalid)
        @(posedge s00_axi_aclk);
    reg_value = s00_axi_rdata;

    @(posedge s00_axi_aclk);
    s00_axi_rready = 1'b0;

    $display ("[%g] AXI_READ Addr: 0x%08h | Value: 0x%08h", $time, reg_addr, reg_value);
end
endtask
    
endmodule
