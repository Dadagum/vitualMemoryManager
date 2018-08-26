#include"VMRunner.h"
#include"RandomUtil.h"
#include"Constant.h"
#include"TwoLevelVmSys.h"
#include"Memory"
#include"Logger.h"

VMRunner::VMRunner()
{
	// 根据题目初始化
	tlb = new Tlb(SysConfig::TLB); // tlb
	process = new Process[5]; // process
	for (int i = 0; i < SysConfig::PROCESS; i++) process[i].setValue(SysConfig::P_SIZE[i], i+1);
	vms = new TwoLevelVmSys; // vms
	ram = new Memory; // ram
}

VMRunner::~VMRunner()
{
	delete tlb;
	delete[] process;
	delete vms;
	delete ram;
}

void VMRunner::request(const Address & address, int pid)
{
	RequestInfo info; // 储存访问信息
	// 提取地址除去offset的部分
	int pNumber = address.getNumber(SysConfig::OFFSET);

	info.pNumber = pNumber; // 记录VP number

	// 检查TLB
	int fNumber = tlb->getfNumber(pNumber);
	if (fNumber == Constant::TLB_MISS) { // TLB缺失

		info.tlbHit = false;

		fNumber = vms->request(address, pid, ram, info); // 在虚存中查找得到页框号
		TlbEntry entry(pNumber, fNumber);
		tlb->allocate(entry); // 记录在TLB中
	}
	else { // TLB命中
		ram->visit(fNumber); // 更新内存的LRU队列

		// 记录TLB,PT命中情况
		info.ptHit = true; 
		info.tlbHit = true;
	}

	// 记录info
	info.fNumber = fNumber; // 记录 PF number
	infoList.push_back(info); // 记在尾部
}

void VMRunner::run()
{
	RandomUtil random;
	for (int r = 0; r < SysConfig::P_ROUND; r++) { // 执行round轮
		for (int p = 1; p < SysConfig::PROCESS; p++) { // 轮流执行 // test : p从1开始
			// 导入外部页表的内容
			vms->importPageTable(p, ram);

			// 得到随机生成序列
			int total = 0;
			Address * addresses = random.nextAddressList(50, total, SysConfig::P_SIZE[p]); // 默认50次循环生成地址
			Logger::saveAddressList(addresses, total, p); // 将访问的虚拟地址输出保存

			for (int i = 0; i < total; i++) {
				request(addresses[i], p); // 访问虚拟地址
			}

			// 导出页表内容
			vms->savePageTable(p);
			// 清空TLB内容
			tlb->clear();
			// 保存命中信息到文件中
			Logger::saveRequestInfo(infoList, 500, p);
		}
	}

}